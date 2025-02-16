#ifndef __HBM_H
#define __HBM_H

#include "DRAM.h"
#include "Request.h"
#include <vector>
#include <functional>

using namespace std;

namespace ramulator
{

class HBM
{
public:
    static string standard_name;
    enum class Org;
    enum class Speed;
    HBM(Org org, Speed speed);
    HBM(const string& org_str, const string& speed_str);

    static map<string, enum Org> org_map;
    static map<string, enum Speed> speed_map;

    /* Level */
    enum class Level : int
    {
        Channel, Rank, BankGroup, Bank, Row, Column, MAX
    };
    static std::string level_str [int(Level::MAX)];
    /* Command */
    enum class Command : int
    {
        ACT, PRE,   PREA,
        RD,  WR,    RDA, WRA,
        REF, REFSB, PDE, PDX,  SRE, SRX,
        GWRITE, G_ACT0, G_ACT1, G_ACT2, G_ACT3, COMP, READRES, //for Newton
        MAX
    };

    // REFSB and REF is not compatible, choose one or the other.
    // REFSB can be issued to banks in any order, as long as REFI1B
    // is satisfied for all banks

    string command_name[int(Command::MAX)] = {
        "ACT", "PRE",   "PREA",
        "RD",  "WR",    "RDA",  "WRA",
        "REF", "REFSB", "PDE",  "PDX",  "SRE", "SRX",
        "GWRITE", "G_ACT0", "G_ACT1", "G_ACT2", "G_ACT3", "COMP", "READRES" //for Newton
    };

    Level scope[int(Command::MAX)] = {
        Level::Row,    Level::Bank,   Level::Rank,
        Level::Column, Level::Column, Level::Column, Level::Column,
        Level::Rank,   Level::Bank,   Level::Rank,   Level::Rank,   Level::Rank,   Level::Rank,
        Level::Rank,   Level::BankGroup, Level::BankGroup, Level::BankGroup, Level::BankGroup, Level::Column, Level::Bank //for Newton
    };

    bool is_BG(Level level)
    {
        switch(int(level)) {
            case int(Level::BankGroup):
                return true;
            default:
                return false;
        }
    }

    bool is_opening(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::ACT):
            case int(Command::G_ACT0):
            case int(Command::G_ACT1):
            case int(Command::G_ACT2):
            case int(Command::G_ACT3):
                return true;
            default:
                return false;
        }
    }

    bool is_pim_opening(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::G_ACT0):
            case int(Command::G_ACT1):
            case int(Command::G_ACT2):
            case int(Command::G_ACT3):
                return true;
            default:
                return false;
        }
    }

    bool is_accessing(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::RD):
            case int(Command::WR):
            case int(Command::RDA):
            case int(Command::WRA):
            case int(Command::COMP):
            //TODO : case int(Command::GWRITE): have to include GWRITE to is_accessing()?
            //TODO : case int(Command::READRES): have to include READRES to is_accessing()?
                return true;
            default:
                return false;
        }
    }

    bool is_pim_accessing(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::COMP):
                return true;
            default:
                return false;
        }
    }

    bool is_closing(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::RDA):
            case int(Command::WRA):
            case int(Command::PRE):
            case int(Command::PREA):
                return true;
            default:
                return false;
        }
    }

    bool is_refreshing(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::REF):
            case int(Command::REFSB):
                return true;
            default:
                return false;
        }
    }

    /* State */
    enum class State : int
    {
        Opened, Closed, PowerUp, ActPowerDown, PrePowerDown, SelfRefresh, MAX
    } start[int(Level::MAX)] = {
        State::MAX, State::PowerUp, State::MAX, State::Closed, State::Closed, State::MAX
    };

    /* Translate */
    Command translate[int(Request::Type::MAX)] = {
        Command::RD,  Command::WR,
        Command::REF, Command::PDE, Command::SRE,
        Command::GWRITE, Command::G_ACT0, Command::G_ACT1, Command::G_ACT2, Command::G_ACT3, Command::COMP, Command::READRES //for Newton
    };

    /* Prereq */
    function<Command(DRAM<HBM>*, Command cmd, int)> prereq[int(Level::MAX)][int(Command::MAX)];

    // SAUGATA: added function object container for row hit status
    /* Row hit */
    function<bool(DRAM<HBM>*, Command cmd, int)> rowhit[int(Level::MAX)][int(Command::MAX)];
    function<bool(DRAM<HBM>*, Command cmd, int)> rowopen[int(Level::MAX)][int(Command::MAX)];

    /* Timing */
    struct TimingEntry
    {
        Command cmd;
        int dist;
        int val;
        bool sibling;
    };
    vector<TimingEntry> timing[int(Level::MAX)][int(Command::MAX)];

    /* Lambda */
    function<void(DRAM<HBM>*, int)> lambda[int(Level::MAX)][int(Command::MAX)];

    /* Organization */
    enum class Org : int
    { // per channel density here. Each stack comes with 8 channels
        HBM_1Gb,
        HBM_2Gb,
        HBM_4Gb,
        HBM_4Gb_bank32,
        HBM_4Gb_bank64,
        HBM_4Gb_bank128,
        HBM_4Gb_bank256,
        HBM_4Gb_bank512,
        MAX
    };

    struct OrgEntry {
        int size;
        int dq;
        int count[int(Level::MAX)];
    } org_table[int(Org::MAX)] = {
        {1<<10, 128, {0, 0, 4, 2, 1<<13, 1<<(6+1)}},
        {2<<10, 128, {0, 0, 4, 2, 1<<14, 1<<(6+1)}},
        {4<<10, 128, {0, 0, 4, 4, 1<<14, 1<<(6+1)}},
        {4<<10, 128, {0, 0, 4, 8, 1<<13, 1<<(6+1)}},
        {4<<10, 128, {0, 0, 4, 16, 1<<12, 1<<(6+1)}},
        {4<<10, 128, {0, 0, 4, 32, 1<<11, 1<<(6+1)}},
        {4<<10, 128, {0, 0, 4, 64, 1<<10, 1<<(6+1)}},
        {4<<10, 128, {0, 0, 4, 128, 1<<9, 1<<(6+1)}},
    }, org_entry;

    void set_channel_number(int channel);
    void set_rank_number(int rank);

    /* Speed */
    enum class Speed : int
    {
        HBM_1Gbps,
        HBM_1Gbps_unlimit_bandwidth,
        MAX
    };

    int prefetch_size = 2; // burst length could be 2 and 4 (choose 4 here), 2n prefetch
    int channel_width = 128;

    struct SpeedEntry {
        int rate;
        double freq, tCK;
        int nRTRS;
        int nBL, nCCDS, nCCDL;
        int nCL, nRCDR, nRCDW, nRP, nCWL;
        int nRAS, nRC;
        int nRTP, nRTPL, nWTRS, nWTRL, nWR;
        int nRRDS, nRRDL, nFAW;
        int nRFC, nREFI, nREFI1B;
        int nPD, nXP;
        int nCKESR, nXS;
        int nRCD_TR_RD;
    } speed_table[int(Speed::MAX)] = {
        {1700,
         850, 1.1764705882352942,
         0,
         1, 1, 2,
         12, 12, 12, 12, 4,
         29, 40,
         4, 6, 6, 7, 14,
         4, 6, 26,
         221, 3315, 109,
         0, 7,
         9, 228,
         0},
        // https://github.com/umd-memsys/DRAMsim3/blob/29817593b3389f1337235d63cac515024ab8fd6e/configs/HBM2_8Gb_x128.ini
        // {2000,
        //  1000, 1.0,
        //  0,
        //  1, 1, 2,
        //  14, 14, 14, 14, 4,
        //  34, 47,
        //  4, 6, 6, 8, 16,
        //  4, 6, 30,
        //  260, 3900, 128,
        //  0, 8,
        //  10, 268,
        //  0},
        // CAL version
        {1700,
        850, 1.1764705882352942,
        2, 2, 3,
        12, 12, 11, 12, 7,
        29, 41,
        12, 4, 7, 14,
        7, 9, 34,
        0, 3315, 0,
        9, 9,
        9, 0},
        // {1000,
        //  500, 2.0,
        //  0,
        //  2, 2, 3,
        //  7, 7, 6, 7, 4,
        //  17, 24,
        //  7, 2, 4, 8,
        //  4, 5, 20,
        //  0, 1950, 0,
        //  5, 5,
        //  5, 0,
        //  0},

        // {2000,
        //  1000, 1.0,
        //  0,
        //  1, 1, 2,
        //  14, 14, 14, 14, 2,
        //  33, 47,
        //  3, 4, 3, 8, 16,
        //  4, 6, 16,
        //  0, 3900, 0,
        //  0, 0,
        //  0, 0,
        //  0},

        // down-scaled from HBM_2Gbps
        // {1752,
        //  876, 1.1415,
        //  0,
        //  1, 1, 2,
        //  12, 12, 12, 12, 2,
        //  29, 41,
        //  3, 4, 3, 7, 14,
        //  4, 5, 14,
        //  0, 3417, 0,
        //  0, 0,
        //  0, 0,
        //  0}
    }, speed_entry;

    int read_latency;
    int readres_latency;

private:
    void init_speed();
    void init_lambda();
    void init_prereq();
    void init_rowhit();  // SAUGATA: added function to check for row hits
    void init_rowopen();
    void init_timing();
};

} /*namespace ramulator*/

#endif /*__HBM_H*/
