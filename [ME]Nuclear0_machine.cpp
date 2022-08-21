#include "avz.h"
#include <queue>
#include <vector>

using namespace AvZ;

#define min(a, b) (((a) < (b)) ? (a) : (b))

struct Zombie_w_No : public Zombie {
    int& No()
    {
        return (int&)((uint8_t*)this)[0x158];
    }
    int& animNo()
    {
        return (int&)((uint8_t*)this)[0x118];
    }
};

struct FieldItem {
public:
    uint8_t data[0xEC];
    int& type()
    {
        return (int&)((uint8_t*)this)[0x8];
    }
    int& row()
    {
        return (int&)((uint8_t*)this)[0x14];
    }
    int& col()
    {
        return (int&)((uint8_t*)this)[0x10];
    }
    int& countDown()
    {
        return (int&)((uint8_t*)this)[0x18];
    }
    bool& isDisappeared()
    {
        return (bool&)((uint8_t*)this)[0x20];
    }
};
struct MainObject_w_fi : public MainObject {
    SafePtr<FieldItem> fieldItemArray()
    {
        return *(FieldItem**)((uint8_t*)this + 0x11C);
    }
    int& fieldItemTotal()
    {
        return (int&)((uint8_t*)this)[0x120];
    }
    int& iceTrailAbscissa(int row)
    {
        return (int&)((uint8_t*)this)[0x60C + row * 4];
    }
    int& iceTrailCountDown(int row)
    {
        return (int&)((uint8_t*)this)[0x624 + row * 4];
    }
};

struct AnimationOffset_plus : public AnimationOffset {
    int& animationTotal()
    {
        return (int&)((uint8_t*)this)[0x4];
    }
};
struct Animation_plus : public Animation {
    int& No()
    {
        return (int&)((uint8_t*)this)[0x9C];
    }
};

int levelMode = 0;

int gridInfo[5][9];
float safeRow[5] = {800.0f, 800.0f, 800.0f, 800.0f, 800.0f};
float safeRowG[5] = {800.0f, 800.0f, 800.0f, 800.0f, 800.0f};
int crushedPot[5][9];

int cherryIndex, jalaIndex, iceIndex, mnuclearIndex, nuclearIndex, cloverIndex, potIndex, pumpkinIndex;
int fshroomIndex, gshroomIndex, sunflowerIndex, twinflowerIndex;
int targetPlace[5][2] = {{1, 5}, {2, 5}, {3, 5}, {0, 3}, {4, 3}};
int critPlace[5][2] = {{3, 4}, {1, 4}, {2, 4}, {4, 3}, {0, 3}};
int flowerPlace[7][2] = {{0, 2}, {4, 2}, {0, 1}, {4, 1}, {2, 2}, {2, 1}, {2, 0}};
int jalaPlace[6][2] = {{1, 5}, {3, 5}, {2, 5}, {1, 5}, {3, 6}, {2, 5}};
int mnuclearPlace[6][2] = {{1, 8}, {3, 7}, {2, 7}, {1, 8}, {3, 7}, {2, 7}};
int nuclearPlace[7][2] = {{1, 6}, {3, 6}, {2, 6}, {3, 5}, {1, 5}, {3, 6}, {2, 6}};
int fastNuclearPlace[6][2] = {{1, 8}, {3, 7}, {2, 7}, {1, 6}, {3, 6}, {2, 6}}; // {1, 7}
int midMNuclearPlace[6][2] = {{1, 8}, {3, 7}, {2, 7}, {1, 7}, {1, 8}, {3, 7}};
int midNuclearPlace[6][2] = {{1, 6}, {3, 6}, {2, 6}, {1, 5}, {1, 6}, {3, 6}};
int jalaFlag = -1;
int jackFlag = 111;
int iceFlag = -1;
bool nuclearFlag = false;
bool jalaBlocker = false;
const int cherryWindow = 200;
const float safeWindow = 40;

int nextPlant = -1;
int realWave = 0;
std::queue<Position> suggestedPositionM;
std::queue<Position> suggestedPositionN;

TickRunner gridViewer;
TickRunner potPadder;

TickRunner AINManeger;
TickRunner NManager;
TickRunner AManager;
TickRunner C3Player;

TickRunner cloverUser;
TickRunner pumpkinPadder;
TickRunner gshroomFixer;
TickRunner sunflowerFixer;

void SetLevelMode()
{
    // 红眼|白眼|冰车|橄榄|气球
    levelMode = 0;
    auto typeList = GetMainObject()->zombieTypeList();
    if (typeList[QQ_16] != 0) {
        levelMode |= 0b1;
    }
    if (typeList[GL_7] != 0) {
        levelMode |= 0b10;
    }
    if (typeList[BC_12] != 0) {
        levelMode |= 0b100;
    }
    if (typeList[BY_23] != 0) {
        levelMode |= 0b1000;
    }
    if (typeList[HY_32] != 0) {
        levelMode |= 0b10000;
    }
    // levelMode = 0b10111;
}

void SelectLevelCards()
{
    std::vector<int> cards;
    // 7张必选
    // 花盆|冰|核|樱桃|南瓜|喷|曾
    cards.push_back(HP_33);
    cards.push_back(HBG_14);
    cards.push_back(HMG_15);
    cards.push_back(YTZD_2);
    cards.push_back(NGT_30);
    cards.push_back(DPG_10);
    cards.push_back(YYG_42);
    if (levelMode & 0b1) {
        cards.push_back(SYC_27);
    }
    if (levelMode >> 1 == 0b11) {
        cards.push_back(M_YTZD_2);
        cards.push_back(HBLJ_20);
    } else {
        if (levelMode >= 0b1000) {
            cards.push_back(M_HMG_15);
            cards.push_back(HBLJ_20);
        } else {
            cards.push_back(M_HBG_14);
        }
    }
    if (cards.size() == 8) {
        cards.push_back(XRK_1);
        cards.push_back(SZXRK_41);
    }
    if (cards.size() == 9) {
        auto plants = GetMainObject()->plantArray();
        auto plantTotal = GetMainObject()->plantTotal();
        for (int i = 0; i < plantTotal; ++i) {
            if (plants[i].type() == XRK_1
                && plants[i].hp() > 0
                && !plants[i].isCrushed()
                && !plants[i].isDisappeared()) {
                cards.push_back(SZXRK_41);
                break;
            }
        }
        if (cards.size() == 9) {
            cards.push_back(XRK_1);
        }
    }
    SelectCards(cards);
}

void GetIndex()
{
    InsertOperation([=]() {
        cherryIndex = GetSeedIndex(YTZD_2);
        jalaIndex = GetSeedIndex(HBLJ_20);
        iceIndex = GetSeedIndex(HBG_14);
        mnuclearIndex = GetSeedIndex(HMG_15, true);
        nuclearIndex = GetSeedIndex(HMG_15);
        cloverIndex = GetSeedIndex(SYC_27);
        potIndex = GetSeedIndex(HP_33);
        pumpkinIndex = GetSeedIndex(NGT_30);
        fshroomIndex = GetSeedIndex(DPG_10);
        gshroomIndex = GetSeedIndex(YYG_42);
        sunflowerIndex = GetSeedIndex(XRK_1);
        twinflowerIndex = GetSeedIndex(SZXRK_41);
    });
}

void DrawGrid()
{
    SafePtr<MainObject_w_fi> mainObj = (MainObject_w_fi*)GetMainObject();
    // plants grid
    int i;
    auto plants = mainObj->plantArray();
    auto plantTotal = mainObj->plantTotal();
    memset(gridInfo, -1, sizeof(gridInfo));
    memset(crushedPot, 0, sizeof(crushedPot));
    for (i = 0; i < plantTotal; ++i) {
        if (plants[i].hp() > 0
            && !plants[i].isCrushed()
            && !plants[i].isDisappeared()) {
            if (plants[i].type() == NGT_30) {
                gridInfo[plants[i].row()][plants[i].col()] += 8;
            } else {
                gridInfo[plants[i].row()][plants[i].col()]++;
            }
        }
        if (plants[i].type() == HP_33
            && plants[i].hp() > 0
            && !plants[i].isDisappeared()) {
            crushedPot[plants[i].row()][plants[i].col()] = 1;
        }
    }
    SafePtr<FieldItem> fItems = mainObj->fieldItemArray();
    int fItemCount = mainObj->fieldItemTotal();
    for (i = 0; i < fItemCount; ++i) {
        if (fItems[i].type() == 2
            && !fItems[i].isDisappeared()
            && fItems[i].countDown() > 0) {
            gridInfo[fItems[i].row()][fItems[i].col()]--;
        }
    }
    int trailLength = 800;
    for (i = 0; i < 5; ++i) {
        trailLength = mainObj->iceTrailAbscissa(i);
        for (int j = (trailLength - 28) / 80; j < 9; ++j) {
            gridInfo[i][j] -= 2;
        }
    }

    // zombies grid
    auto zombies = mainObj->zombieArray();
    int zombieCount = mainObj->zombieTotal();
    jackFlag = 111;
    for (i = 0; i < 5; ++i) {
        safeRow[i] = 800.0f;
        safeRowG[i] = 800.0f;
    }
    for (i = 0; i < zombieCount; ++i) {
        if (zombies[i].type() == XC_15
            && zombies[i].hp() > 0
            && !zombies[i].isDead()
            && zombies[i].isExist()
            && !zombies[i].isDisappeared()
            && zombies[i].state() == 16) {
            int jackTime = zombies[i].stateCountdown();
            if (jackTime < jackFlag) {
                jackFlag = jackTime;
            }
        }
        if ((zombies[i].type() == HY_32 || zombies[i].type() == BY_23)
            && zombies[i].hp() > 0
            && !zombies[i].isDead()
            && zombies[i].isExist()
            && !zombies[i].isDisappeared()) {
            int row = zombies[i].row();
            float abscissa = zombies[i].abscissa();
            if (abscissa < safeRow[row]) {
                safeRow[row] = abscissa;
                safeRowG[row] = abscissa;
            }
        }
        if (zombies[i].type() == BC_12
            && zombies[i].hp() > 0
            && !zombies[i].isDead()
            && zombies[i].isExist()
            && !zombies[i].isDisappeared()) {
            int row = zombies[i].row();
            float abscissa = zombies[i].abscissa() + 40.0f;
            if (abscissa < safeRow[row]) {
                safeRow[row] = abscissa;
            }
        }
    }
    if (jackFlag == 110) {
        ShowErrorNotInQueue("Jack Alarm!");
    }
}

bool IsUsableIn(int seedIndex, int time)
{
    auto seeds = GetMainObject()->seedArray();
    if (seeds[seedIndex].cd() == 0 || seeds[seedIndex].initialCd() - seeds[seedIndex].cd() + 1 <= time) {
        return true;
    }
    return false;
}

bool RuinIsUsableIn(int row, int col, int time)
{
    SafePtr<FieldItem> fItems = ((MainObject_w_fi*)GetMainObject())->fieldItemArray();
    int fItemCount = ((MainObject_w_fi*)GetMainObject())->fieldItemTotal();
    for (int i = 0; i < fItemCount; ++i) {
        if (fItems[i].type() == 2
            && !fItems[i].isDisappeared()
            && fItems[i].row() == row
            && fItems[i].col() == col
            && fItems[i].countDown() > time) {
            return false;
        }
    }
    return true;
}

float FindAnimation(int animNo)
{
    SafePtr<Animation_plus> animation = (Animation_plus*)(GetPvzBase()
                                                              ->animationMain()
                                                              ->animationOffset()
                                                              ->animationArray());
    int animationCount = ((AnimationOffset_plus*)(GetPvzBase()
                                                      ->animationMain()
                                                      ->animationOffset()))
                             ->animationTotal();
    for (int i = 0; i < animationCount; ++i) {
        if (animation[i].No() == animNo) {
            return animation[i].circulationRate();
        }
    }
    return -1;
}

inline float RowMean()
{
    return (safeRow[0] + safeRow[1] + safeRow[2] + safeRow[3] + safeRow[4]) / 5.0f;
}

inline float RowMin()
{
    return min(min(min(min(safeRow[0], safeRow[1]), safeRow[2]), safeRow[3]), safeRow[4]);
}

inline void SafePlacePot(int row, int col)
{
    if (gridInfo[row - 1][col - 1] == -1 && crushedPot[row - 1][col - 1] == 0) {
        CardNotInQueue(potIndex + 1, row, col);
    }
}

void JudgeIce(bool m = false)
{
    int row = 0, col = 0, i;
    int cardIndex = GetSeedIndex(HBG_14, m);
    // for (i = 0; i < 5; ++i) {
    //     row = targetPlace[i][0];
    //     col = targetPlace[i][1];
    //     if (gridInfo[row][col] % 8 == 0) {
    //         CardNotInQueue(cardIndex + 1, row + 1, col + 1);
    //         return;
    //     }
    // }
    for (row = 0; row < 5; ++row) {
        for (col = 0; col < 6; ++col) {
            if (gridInfo[row][col] % 8 == 0) {
                CardNotInQueue(cardIndex + 1, row + 1, col + 1);
                return;
            }
        }
    }
    if (levelMode < 0b01000) {
        auto seeds = GetMainObject()->seedArray();
        if (seeds[potIndex].isUsable()) {
            if (gridInfo[0][4] % 8 != 0) {
                ShovelNotInQueue(1, 5);
            }
            CardNotInQueue(potIndex + 1, 1, 5);
            CardNotInQueue(cardIndex + 1, 1, 5);
            return;
        }
    }
    for (i = 4; i >= 0; --i) {
        row = targetPlace[i][0];
        col = targetPlace[i][1];
        if (gridInfo[row][col] > 0) {
            if (GetPlantIndex(row + 1, col + 1, DPG_10) >= 0) {
                ShovelNotInQueue(row + 1, col + 1);
                CardNotInQueue(cardIndex + 1, row + 1, col + 1);
                return;
            }
        }
    }
}

void JudgeJala()
{
    if (gridInfo[3][6] == -1) {
        return;
    }
    jalaPlace[4][0] = 1;
    mnuclearPlace[4][0] = 1;
    mnuclearPlace[4][1] = 6;
    nuclearPlace[5][0] = 4;
    nuclearPlace[5][1] = 4;
}

void AIN()
{
    int time = -65535;
    int placeIndex, jalaPlaceRow, jalaPlaceCol, mnuclearPlaceRow, mnuclearPlaceCol;
    auto seeds = GetMainObject()->seedArray();
    if (jalaFlag > 0) {
        --jalaFlag;
    }
    if (iceFlag > 0) {
        --iceFlag;
    }
    time = NowTime(20);
    if (time >= 0) {
        if (seeds[iceIndex].isUsable()
            && IsUsableIn(mnuclearIndex, 90)
            && IsUsableIn(potIndex, 90)
            && RuinIsUsableIn(1, 7, 90)
            && gridInfo[1][7] >= -2
            && nuclearFlag == true) {
            if (jackFlag == 111) {
                JudgeIce();
                ShowErrorNotInQueue("Ice! time: #, wave: 20", time);
                SetTime(time + 90, 20);
                Card(HP_33, 2, 8);
                Card(M_HMG_15, 2, 8);
                SetTime(time + 91, 20);
                potPadder.goOn();
                return;
            }
        }
        return;
    }

    time = NowTime(9);
    if (time >= 0 && NowTime(10) < -100) {
        if (seeds[iceIndex].isUsable()) {
            JudgeIce();
        }
        return;
    }

    for (auto wave : {19, 16, 13, 8, 5, 2}) {
        time = NowTime(wave);
        if (time >= 0 && time < 4500) {
            placeIndex = (wave - 2) / 3;
            jalaPlaceRow = jalaPlace[placeIndex][0];
            jalaPlaceCol = jalaPlace[placeIndex][1];
            if (seeds[jalaIndex].isUsable()
                && (gridInfo[jalaPlaceRow][jalaPlaceCol] % 8 == 0
                    || (gridInfo[jalaPlaceRow][jalaPlaceCol] != -2
                        && gridInfo[jalaPlaceRow][jalaPlaceCol] != -4
                        && seeds[potIndex].isUsable()))
                && time >= 226) {
                if (jackFlag > 100) {
                    if (wave == 16) {
                        JudgeJala();
                        jalaPlaceRow = jalaPlace[placeIndex][0];
                        // protect mnuclear
                        SetTime(time + 100, 16);
                        Shovel(jalaPlaceRow + 1, jalaPlaceCol + 1);
                    }
                    CardNotInQueue(potIndex + 1, jalaPlaceRow + 1, jalaPlaceCol + 1);
                    CardNotInQueue(jalaIndex + 1, jalaPlaceRow + 1, jalaPlaceCol + 1);
                    jalaFlag = 100;
                    ShowErrorNotInQueue("Jala! time: #, wave: #", time, wave);
                    return;
                }
            }
            mnuclearPlaceRow = mnuclearPlace[placeIndex][0];
            mnuclearPlaceCol = mnuclearPlace[placeIndex][1];
            if (jalaFlag == 0
                && time >= 5000 - 1600 - 1254 - 200 - 420 - 90 - 4 * cherryWindow
                && seeds[iceIndex].isUsable()
                && IsUsableIn(mnuclearIndex, 90)
                && IsUsableIn(potIndex, 90)
                && RuinIsUsableIn(mnuclearPlaceRow, mnuclearPlaceCol, 90)) {
                if (wave == 19 && time > 1947 - 420 - 90) {
                    if (RowMean() > 6.5 * 80) {
                        jalaFlag = -1;
                        return;
                    }
                }
                if (IsUsableIn(nuclearIndex, 90 + 420 + 200 + 1054 - 100)
                    && IsUsableIn(cherryIndex, 90 + 420 + 200 + 1054 + 200 + 1555 - 100)) {
                    if (jackFlag == 111) {
                        JudgeIce();
                        jalaFlag = -1;
                        iceFlag = 90;
                        ShowErrorNotInQueue("Ice! time: #, wave: #", time, wave);
                        // SetTime(time + 91, wave);
                        // potPadder.goOn();
                        return;
                    }
                }
            }
            if (iceFlag == 0) {
                if (jackFlag == 111) {
                    CardNotInQueue(potIndex + 1, mnuclearPlaceRow + 1, mnuclearPlaceCol + 1);
                    CardNotInQueue(mnuclearIndex + 1, mnuclearPlaceRow + 1, mnuclearPlaceCol + 1);
                    iceFlag = -1;
                    ShowErrorNotInQueue("MNuclear! time: #, wave: #", time, wave);
                    if (wave == 19) {
                        SetNowTime();
                        potPadder.goOn();
                    }
                    return;
                }
            }
            return;
        }
    }
}

void Nuclear()
{
    int time = -65535;
    int placeIndex, placeRow, placeCol;
    auto seeds = GetMainObject()->seedArray();

    for (auto wave : {20, 17, 14, 11, 9, 6, 3}) {
        time = NowTime(wave);
        if (wave == 20 && time > 394 - 100) {
            time = 1054 - 50 - 0.5 * cherryWindow;
        }
        if (time >= 1054 - 50 - 0.5 * cherryWindow && time < 4500) {
            if (wave == 20) {
                time = NowTime(20);
            }
            if (wave == 17 && jalaPlace[4][0] == 1 && time < 1555 - 100) {
                return;
            }
            if (wave == 11 && time < 1352 - cherryWindow) {
                return;
            }

            placeIndex = (wave - 1) / 3;
            placeRow = nuclearPlace[placeIndex][0];
            placeCol = nuclearPlace[placeIndex][1];
            if (seeds[nuclearIndex].isUsable()
                && (gridInfo[placeRow][placeCol] % 8 == 0 || (gridInfo[placeRow][placeCol] == -1 && seeds[potIndex].isUsable()))) {
                if (wave == 20 || IsUsableIn(cherryIndex, 50 + 200 + 1455)) {
                    if (jackFlag > 100) {
                        CardNotInQueue(potIndex + 1, placeRow + 1, placeCol + 1);
                        CardNotInQueue(nuclearIndex + 1, placeRow + 1, placeCol + 1);
                        ShowErrorNotInQueue("Nuclear! time: #, wave: #", time, wave);
                        if (wave == 20) {
                            nuclearFlag = true;
                        } else {
                            SetTime(time + 1, wave);
                            potPadder.goOn();
                        }
                        return;
                    }
                }
            }
            return;
        }
    }
}

void Cherry()
{
    int time = -65535;

    time = NowTime(9);
    if (time >= 1300 && NowTime(10) == -65535) {
        auto seeds = GetMainObject()->seedArray();
        if (IsUsableIn(cherryIndex, 751)) {
            SetTime(time, 9);
            potPadder.pause();
        }
        if (seeds[cherryIndex].isUsable()
            && (gridInfo[2][5] % 8 == 0 || (gridInfo[2][5] == -1 && seeds[potIndex].isUsable()))) {
            if (jackFlag > 100) {
                CardNotInQueue(potIndex + 1, 3, 6);
                CardNotInQueue(cherryIndex + 1, 3, 6);
                ShowErrorNotInQueue("Mid Cherry! time: #, wave: 9", time);
                SetTime(time + 1, 9);
                potPadder.goOn();
                return;
            }
        }
        return;
    }

    for (auto wave : {18, 15, 12, 7, 4}) {
        time = NowTime(wave);
        if (time >= 1054 && time < 4500) {
            auto seeds = GetMainObject()->seedArray();
            if (seeds[cherryIndex].isUsable()
                && (gridInfo[2][5] % 8 == 0 || (gridInfo[2][5] == -1 && seeds[potIndex].isUsable()))
                && IsUsableIn(jalaIndex, 50 + 200 + 1947 - 420 - 751 + cherryWindow)
                && IsUsableIn(mnuclearIndex, 50 + 200 + 1947 - 420)
                && IsUsableIn(nuclearIndex, 50 + 200 + 1947 + 200 + 1054 - 100 + cherryWindow)) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, 3, 6);
                    CardNotInQueue(cherryIndex + 1, 3, 6);
                    ShowErrorNotInQueue("Cherry! time: #, wave: #", time, wave);
                    SetTime(time + 1, wave);
                    // if (wave != 12) {
                    potPadder.goOn();
                    // }
                    return;
                }
            }
            return;
        }
    }
}

void C3()
{
    int time = -65535;
    auto seeds = GetMainObject()->seedArray();
    int mIceIndex = GetSeedIndex(HBG_14, true);

    time = NowTime(20);
    if (time > 394 - 319 - 100 && time < 1000) {
        if (seeds[mIceIndex].isUsable()) {
            if (jackFlag == 111) {
                JudgeIce(true);
            }
        }
        return;
    }

    time = NowTime(10);
    if (time > 394 - 319 - 100 && NowTime(11) == -65535) {
        if (seeds[mIceIndex].isUsable()) {
            if (jackFlag == 111) {
                JudgeIce(true);
            }
        }
        return;
    }

    for (auto wave : {19, 16, 13, 7, 4, 1}) {
        time = NowTime(wave);
        if (time > -100 && NowTime(wave + 1) == -65535) {
            if (seeds[iceIndex].isUsable()) {
                if (jackFlag > 100) {
                    JudgeIce();
                }
            }
            break;
        }
    }
    for (auto wave : {18, 15, 12, 9, 6, 3}) {
        time = NowTime(wave);
        if (time > 486 && NowTime(wave + 1) == -65535) {
            int row = fastNuclearPlace[wave / 3 - 1][0];
            int col = fastNuclearPlace[wave / 3 - 1][1];
            if (seeds[nuclearIndex].isUsable()
                && seeds[potIndex].isUsable()
                && gridInfo[row][col] == -1) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, row + 1, col + 1);
                    CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                }
            }
            break;
        }
    }
    for (auto wave : {17, 14, 11, 8, 5, 2}) {
        time = NowTime(wave);
        if (time > 1352 - 100 && NowTime(wave + 1) == -65535) {
            if (seeds[cherryIndex].isUsable()
                && (seeds[potIndex].isUsable() || gridInfo[2][5] % 8 == 0)) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, 3, 6);
                    CardNotInQueue(cherryIndex + 1, 3, 6);
                }
            }
            break;
        }
    }
}

void C3plus()
{
    int time = -65535;
    auto seeds = GetMainObject()->seedArray();
    int mcherryIndex = GetSeedIndex(YTZD_2, true);

    if (jalaFlag > 0) {
        --jalaFlag;
    }
    if (iceFlag > 0) {
        --iceFlag;
    }

    for (auto wave : {20, 17, 14, 11, 8, 5, 2}) {
        time = NowTime(wave);
        if (time > 200 && time < 4500) {
            if (wave == 20 && time <= 394 - 100) {
                return;
            }
            if (seeds[iceIndex].isUsable()
                && time > 1555 - 100 - 420 - 90
                && IsUsableIn(mcherryIndex, 90)
                && IsUsableIn(potIndex, 90)) {
                if (jackFlag == 111) {
                    JudgeIce();
                    iceFlag = 100;
                }
            }
            if (iceFlag == 0 && time > 1555 - 100 - 420) {
                if (seeds[mcherryIndex].isUsable()
                    && (seeds[potIndex].isUsable() || gridInfo[2][5] % 8 == 0)) {
                    if (jackFlag == 111) {
                        if (safeRow[2] >= 530.0F + 40.0F) {
                            CardNotInQueue(potIndex + 1, 3, 6);
                            CardNotInQueue(mcherryIndex + 1, 3, 6);
                        } else {
                            ShovelNotInQueue(3, 5);
                            CardNotInQueue(potIndex + 1, 3, 5);
                            CardNotInQueue(mcherryIndex + 1, 3, 5);
                        }
                        SetNowTime();
                        potPadder.goOn();
                    }
                }
            }
            break;
        }
    }

    for (auto wave : {19, 16, 13, 10, 7, 4, 1}) {
        time = NowTime(wave);
        if (time > 1555 - 200 && time < 4500) {
            if (seeds[cherryIndex].isUsable()
                && (seeds[potIndex].isUsable() || gridInfo[2][5] % 8 == 0)) {
                if (IsUsableIn(mcherryIndex, 50 + 200 + 1555 - 320)) {
                    if (jackFlag > 100) {
                        CardNotInQueue(potIndex + 1, 3, 6);
                        CardNotInQueue(cherryIndex + 1, 3, 6);
                        SetNowTime();
                        potPadder.goOn();
                    }
                }
            }
            break;
        }
    }

    for (auto wave : {18, 15, 12, 9, 6, 3}) {
        time = NowTime(wave);
        if (time > 486 && time < 4500) {
            int index = wave / 3 - 1;
            int row = jalaPlace[index][0];
            int col = jalaPlace[index][1];
            if (seeds[jalaIndex].isUsable()
                && (seeds[potIndex].isUsable() || gridInfo[row][col] % 8 == 0)) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, row + 1, col + 1);
                    CardNotInQueue(jalaIndex + 1, row + 1, col + 1);
                    jalaFlag = 100;
                }
            }
            row = fastNuclearPlace[index][0];
            col = fastNuclearPlace[index][1];
            if (jalaFlag == 0
                && seeds[nuclearIndex].isUsable()
                && seeds[potIndex].isUsable()
                && gridInfo[row][col] == -1) {
                if (IsUsableIn(cherryIndex, 50 + 200 + 1555)) {
                    if (jackFlag > 100) {
                        CardNotInQueue(potIndex + 1, row + 1, col + 1);
                        CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                        jalaFlag = -1;
                        SetNowTime();
                        potPadder.goOn();
                    }
                }
            }
            break;
        }
    }
}

void C3minus()
{
    int time = -65535;
    auto seeds = GetMainObject()->seedArray();

    time = NowTime(20);
    if (time > 394 - 100 + 1) {
        if (seeds[nuclearIndex].isUsable()
            && seeds[potIndex].isUsable()
            && gridInfo[4][5] == -1) {
            CardNotInQueue(potIndex + 1, 5, 6);
            CardNotInQueue(nuclearIndex + 1, 5, 6);
            SetNowTime();
            potPadder.goOn();
        }
        return;
    }

    for (auto wave : {15, 12, 5, 2}) {
        time = NowTime(wave);
        if (time > 1352 - 100 && NowTime(wave + 1) == -65535) {
            int row, col = 5;
            switch (wave) {
            case 2:
            case 12:
                row = 1;
                break;
            default:
                row = 3;
                break;
            }
            if (seeds[nuclearIndex].isUsable()
                && seeds[potIndex].isUsable()
                && gridInfo[row][col] == -1) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, row + 1, col + 1);
                    CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                    SetNowTime();
                    potPadder.goOn();
                }
            }
            break;
        }
    }
    for (auto wave : {18, 8}) {
        time = NowTime(wave);
        if (time > 1352 - 100 && NowTime(wave + 1) == -65535) {
            if (seeds[cherryIndex].isUsable()
                && (seeds[potIndex].isUsable() || gridInfo[2][5] % 8 == 0)) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, 3, 6);
                    CardNotInQueue(cherryIndex + 1, 3, 6);
                    SetNowTime();
                    potPadder.goOn();
                    SetTime(time + 100, wave);
                    Shovel(3, 6);
                }
            }
            break;
        }
    }
    if (NowTime(2) == -65535 && NowTime(1) > -100) {
        if (seeds[iceIndex].isUsable()) {
            JudgeIce();
        }
    }
}

void C3sharp()
{
    int time = -65535;
    auto seeds = GetMainObject()->seedArray();

    if (jalaFlag > 0) {
        --jalaFlag;
    }
    if (iceFlag > 0) {
        --iceFlag;
    }

    time = NowTime(20);
    if (time > 394 - 100 + 1 && time < 4500) {
        if (seeds[iceIndex].isUsable()) {
            if (jackFlag == 111) {
                JudgeIce();
                iceFlag = 90;
                ShowErrorNotInQueue("Ice! time: #, wave: 20", time);
                return;
            }
        }
        if (iceFlag == 0
            && seeds[mnuclearIndex].isUsable()
            && seeds[potIndex].isUsable()
            && gridInfo[4][6] == -1) {
            if (jackFlag == 111) {
                CardNotInQueue(potIndex + 1, 5, 7);
                CardNotInQueue(mnuclearIndex + 1, 5, 7);
                iceFlag = -1;
                ShowErrorNotInQueue("MNuclear! time: #, wave: 20", time);
                SetTime(time + 1, 20);
                potPadder.goOn();
                return;
            }
        }
        return;
    }

    for (auto wave : {17, 14, 11, 8, 5, 2}) {
        time = NowTime(wave);
        if (time >= 0 && time < 4500) {
            int index = wave / 3;
            int row = jalaPlace[index][0];
            int col = jalaPlace[index][1];
            if (seeds[jalaIndex].isUsable()
                && (gridInfo[row][col] % 8 == 0 || (gridInfo[row][col] == -1 && seeds[potIndex].isUsable()))
                && time >= 226) {
                if (levelMode & 0b100 || wave == 2 || wave == 14) {
                    if (jackFlag > 100) {
                        CardNotInQueue(potIndex + 1, row + 1, col + 1);
                        CardNotInQueue(jalaIndex + 1, row + 1, col + 1);
                        jalaFlag = 100;
                        ShowErrorNotInQueue("Jala! time: #, wave: #", time, wave);
                        return;
                    }
                } else {
                    if (jalaBlocker == false) {
                        jalaFlag = 0;
                        jalaBlocker = true;
                    }
                }
            }
            row = midMNuclearPlace[index][0];
            col = midMNuclearPlace[index][1];
            if (jalaFlag == 0
                && time >= 5000 - 1600 - 1254 - 200 - 420 - 90 - 3 * cherryWindow
                && seeds[iceIndex].isUsable()
                && IsUsableIn(mnuclearIndex, 90)
                && IsUsableIn(potIndex, 90)
                && RuinIsUsableIn(row, col, 90)) {
                if (IsUsableIn(nuclearIndex, 90 + 420 + 200 + 1054 - 100)
                    && IsUsableIn(cherryIndex, 90 + 420 + 200 + 1054 + 200 + 1555 - 100)) {
                    if (jackFlag == 111) {
                        JudgeIce();
                        jalaFlag = -1;
                        iceFlag = 90;
                        ShowErrorNotInQueue("Ice! time: #, wave: #", time, wave);
                        return;
                    }
                }
            }
            if (iceFlag == 0) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, row + 1, col + 1);
                    CardNotInQueue(mnuclearIndex + 1, row + 1, col + 1);
                    iceFlag = -1;
                    ShowErrorNotInQueue("MNuclear! time: #, wave: #", time, wave);
                    return;
                }
            }
            break;
        }
    }

    for (auto wave : {19, 16, 13, 10, 7, 4}) {
        time = NowTime(wave);
        if (time >= 1054 && time < 4500) {
            if (wave == 10 && time < 1352) {
                return;
            }
            auto seeds = GetMainObject()->seedArray();
            if (seeds[cherryIndex].isUsable()
                && (gridInfo[2][5] % 8 == 0 || (gridInfo[2][5] == -1 && seeds[potIndex].isUsable()))
                && IsUsableIn(jalaIndex, 50 + 200 + 1947 - 420 - 751 + cherryWindow)
                && IsUsableIn(mnuclearIndex, 50 + 200 + 1947 - 420)
                && IsUsableIn(nuclearIndex, 50 + 200 + 1947 + 200 + 1054 - 100 + cherryWindow)) {
                if (jackFlag > 100) {
                    CardNotInQueue(potIndex + 1, 3, 6);
                    CardNotInQueue(cherryIndex + 1, 3, 6);
                    ShowErrorNotInQueue("Cherry! time: #, wave: #", time, wave);
                    SetTime(time + 1, wave);
                    potPadder.goOn();
                    return;
                }
            }
            break;
        }
    }

    for (auto wave : {18, 15, 12, 9, 6, 3}) {
        time = NowTime(wave);
        if (time >= 1054 - 50 - cherryWindow && time < 4500) {
            int index = wave / 3 - 1;
            int row = midNuclearPlace[index][0];
            int col = midNuclearPlace[index][1];
            if (seeds[nuclearIndex].isUsable()
                && (gridInfo[row][col] % 8 == 0 || (gridInfo[row][col] == -1 && seeds[potIndex].isUsable()))) {
                if (IsUsableIn(cherryIndex, 50 + 200 + 1455)) {
                    if (jackFlag > 100) {
                        CardNotInQueue(potIndex + 1, row + 1, col + 1);
                        CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                        ShowErrorNotInQueue("Nuclear! time: #, wave: #", time, wave);
                        SetTime(time + 1, wave);
                        potPadder.goOn();
                        return;
                    }
                }
            }
            break;
        }
    }
}

void C3change()
{
    int time = -65535;
    int row = 0, col = 0;
    float rowArg = 800.0f;
    auto seeds = GetMainObject()->seedArray();
    if (jalaFlag > 0) {
        --jalaFlag;
    }
    if (iceFlag > 0) {
        --iceFlag;
    }

    time = NowTime(20);
    if (time > 0 && realWave == 20 && jalaFlag == -1 && iceFlag == -1) {
        if (potPadder.getStatus() == RUNNING) {
            SetNowTime();
            potPadder.pause();
        }
        // col = 7;
        // if (safeRowG[0] > safeRowG[4]) {
        //     row = 0;
        // } else {
        //     row = 4;
        // }
        switch (nextPlant) {
        case 0:
            row = 2;
            col = 7;
            if (seeds[jalaIndex].isUsable() && time > 226) {
                if (gridInfo[row][5] % 8 == 0 || gridInfo[row][5] == -1 && seeds[potIndex].isUsable()) {
                    CardNotInQueue(potIndex + 1, row + 1, 6);
                    CardNotInQueue(jalaIndex + 1, row + 1, 6);
                    ShowErrorNotInQueue("Jala! time: #, wave: 20", time);
                    return;
                }
            }
            if (seeds[iceIndex].isUsable()) {
                if (IsUsableIn(mnuclearIndex, 90)
                    && IsUsableIn(potIndex, 90)
                    && gridInfo[row][col] == -1) {
                    if (jackFlag == 111) {
                        JudgeIce();
                        ShowErrorNotInQueue("Ice! time: #, wave: 20", time);
                        SetTime(time + 90, 20);
                        Card(HP_33, row + 1, col + 1);
                        Card(M_HMG_15, row + 1, col + 1);
                        ShowError("MNuclear! time: #, wave: 20", time + 90);
                        potPadder.goOn();
                        ++realWave;
                        return;
                    }
                }
            }
            break;
        case 1:
            row = 2;
            col = 6;
            if (seeds[nuclearIndex].isUsable() && seeds[potIndex].isUsable()) {
                if (gridInfo[row][col] == -1) {
                    if (jackFlag > 100) {
                        CardNotInQueue(potIndex + 1, row + 1, col + 1);
                        CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                        ShowErrorNotInQueue("Nuclear! time: #, wave: 20", time);
                        SetNowTime();
                        potPadder.goOn();
                        ++realWave;
                        return;
                    }
                }
            }
            break;
        case 2:
            row = 1;
            col = 7;
            if (seeds[iceIndex].isUsable()) {
                if (IsUsableIn(mnuclearIndex, 90)
                    && IsUsableIn(potIndex, 90)
                    && gridInfo[row][col] == -1) {
                    if (jackFlag == 111) {
                        JudgeIce();
                        ShowErrorNotInQueue("Ice! time: #, wave: 20", time);
                        SetTime(time + 90, 20);
                        Card(HP_33, row + 1, col + 1);
                        Card(M_HMG_15, row + 1, col + 1);
                        ShowError("MNuclear! time: #, wave: 20", time + 90);
                        potPadder.goOn();
                        ++realWave;
                        return;
                    }
                }
            }
            break;
        default:
            break;
        }
    }

    time = NowTime(10);
    if (time != -65535 && realWave <= 10 && NowTime(11) == -65535) {
        if (realWave == 9) {
            while (!suggestedPositionM.empty()) {
                suggestedPositionM.pop();
            }
            while (!suggestedPositionN.empty()) {
                suggestedPositionN.pop();
            }
            switch (nextPlant) {
            case 0:
                suggestedPositionM.push({1, 7});
                suggestedPositionM.push({3, 8});
                suggestedPositionM.push({3, 6});
                suggestedPositionM.push({2, 7});
                suggestedPositionN.push({1, 5});
                suggestedPositionN.push({3, 5});
                suggestedPositionN.push({4, 4});
                suggestedPositionN.push({2, 6});
                break;
            case 1:
                suggestedPositionM.push({1, 7});
                suggestedPositionM.push({3, 8});
                suggestedPositionM.push({3, 6});
                suggestedPositionN.push({2, 6});
                suggestedPositionN.push({1, 5});
                suggestedPositionN.push({3, 5});
                suggestedPositionN.push({4, 4});
                break;
            case 2:
                suggestedPositionM.push({1, 7});
                suggestedPositionM.push({3, 8});
                suggestedPositionM.push({3, 6});
                suggestedPositionM.push({2, 7});
                suggestedPositionN.push({1, 5});
                suggestedPositionN.push({3, 5});
                suggestedPositionN.push({4, 4});
                break;
            }
            ++realWave;
        }
        switch (nextPlant) {
        case 0:
            if (seeds[mnuclearIndex].isUsable() && seeds[potIndex].isUsable()) {
                row = suggestedPositionM.front().row;
                col = suggestedPositionM.front().col;
                if (time >= -725 + 751
                    && gridInfo[row][col] == -1) {
                    if (jackFlag == 111) {
                        CardNotInQueue(potIndex + 1, row + 1, col + 1);
                        CardNotInQueue(mnuclearIndex + 1, row + 1, col + 1);
                        ShowErrorNotInQueue("MNuclear! time: #, wave: 10", time);
                        SetNowTime();
                        potPadder.goOn();
                        realWave = 11;
                        ++nextPlant;
                        SetTime(1054, 11);
                        InsertOperation([=]() {
                            realWave = 10;
                        });
                        SetTime(1555 - 100 - 751, 11);
                        potPadder.pause();
                        suggestedPositionM.pop();
                        jalaFlag = -1;
                        iceFlag = -1;
                        return;
                    }
                }
            }
            break;
        case 1:
            if (seeds[nuclearIndex].isUsable() && seeds[potIndex].isUsable()) {
                if (time >= 1054 - 100) {
                    row = suggestedPositionN.front().row;
                    col = suggestedPositionN.front().col;
                    if (gridInfo[row][col] == -1 || gridInfo[row][col] % 8 == 0) {
                        if (jackFlag > 100) {
                            CardNotInQueue(potIndex + 1, row + 1, col + 1);
                            CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                            ShowErrorNotInQueue("Nuclear! time: #, wave: 10", time);
                            SetNowTime();
                            potPadder.goOn();
                            realWave = 11;
                            ++nextPlant;
                            SetTime(1054, 11);
                            InsertOperation([=]() {
                                realWave = 10;
                            });
                            SetTime(1555 - 100 - 751, 11);
                            potPadder.pause();
                            suggestedPositionN.pop();
                            return;
                        }
                    }
                }
            }
            break;
        case 2:
            if (seeds[cherryIndex].isUsable()) {
                if (time >= 1300) {
                    if (gridInfo[2][5] % 8 == 0 || (gridInfo[2][5] == -1 && seeds[potIndex].isUsable())) {
                        if (jackFlag > 100) {
                            CardNotInQueue(potIndex + 1, 3, 6);
                            CardNotInQueue(cherryIndex + 1, 3, 6);
                            ShowErrorNotInQueue("Cherry! time: #, wave: 10", time);
                            SetNowTime();
                            potPadder.goOn();
                            realWave = 11;
                            nextPlant = 0;
                            SetTime(226, 11);
                            InsertOperation([=]() {
                                realWave = 10;
                            });
                            SetTime(1947 - 420 - 751 - 751, 11);
                            potPadder.pause();
                            return;
                        }
                    }
                }
            }
            break;
        default:
            break;
        }
        return;
    }

    time = NowTime(9);
    if (time > 0 && realWave == 9 && jalaFlag == -1 && iceFlag == -1) {
        if (potPadder.getStatus() == RUNNING) {
            SetNowTime();
            potPadder.pause();
        }
        if (RowMin() <= 6 * 80 || RowMean() <= 7 * 80) {
            switch (nextPlant) {
            case 2:
                if (seeds[cherryIndex].isUsable()) {
                    if (gridInfo[2][5] % 8 == 0 || (gridInfo[2][5] == -1 && seeds[potIndex].isUsable())) {
                        if (jackFlag > 100) {
                            CardNotInQueue(potIndex + 1, 3, 6);
                            CardNotInQueue(cherryIndex + 1, 3, 6);
                            ShowErrorNotInQueue("Cherry! time: #, wave: 9", time);
                            if (NowTime(10) == -65535) {
                                SetNowTime();
                                potPadder.goOn();
                            }
                            SetTime(-725, 10);
                            potPadder.pause();

                            ++realWave;
                            nextPlant = 0;

                            while (!suggestedPositionM.empty()) {
                                suggestedPositionM.pop();
                            }
                            while (!suggestedPositionN.empty()) {
                                suggestedPositionN.pop();
                            }
                            suggestedPositionM.push({1, 7});
                            suggestedPositionM.push({3, 8});
                            suggestedPositionM.push({3, 6});
                            suggestedPositionM.push({2, 7});
                            suggestedPositionN.push({1, 5});
                            suggestedPositionN.push({3, 5});
                            suggestedPositionN.push({4, 4});
                            suggestedPositionN.push({2, 6});

                            jalaFlag = 0;
                            iceFlag = -2;
                            return;
                        }
                    }
                }
                break;
            case 1:
                if (seeds[nuclearIndex].isUsable() && seeds[potIndex].isUsable()) {
                    row = suggestedPositionN.front().row;
                    col = suggestedPositionN.front().col;
                    if (gridInfo[row][col] == -1 || gridInfo[row][col] % 8 == 0) {
                        if (jackFlag > 100) {
                            CardNotInQueue(potIndex + 1, row + 1, col + 1);
                            CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                            ShowErrorNotInQueue("Nuclear! time: #, wave: 9", time);
                            SetNowTime();
                            potPadder.goOn();
                            SetTime(1555 - 100 - 751, 10);
                            potPadder.pause();

                            ++realWave;
                            ++nextPlant;

                            while (!suggestedPositionM.empty()) {
                                suggestedPositionM.pop();
                            }
                            while (!suggestedPositionN.empty()) {
                                suggestedPositionN.pop();
                            }
                            suggestedPositionM.push({1, 7});
                            suggestedPositionM.push({3, 8});
                            suggestedPositionM.push({3, 6});
                            suggestedPositionM.push({2, 7});
                            suggestedPositionN.push({1, 5});
                            suggestedPositionN.push({3, 5});
                            suggestedPositionN.push({4, 4});
                            return;
                        }
                    }
                }
                break;
            case 0:
                if (seeds[iceIndex].isUsable()) {
                    if (IsUsableIn(mnuclearIndex, 90) && IsUsableIn(potIndex, 90)) {
                        row = suggestedPositionM.front().row;
                        col = suggestedPositionM.front().col;
                        if (gridInfo[row][col] == -1) {
                            if (jackFlag == 111) {
                                JudgeIce();
                                ShowErrorNotInQueue("Ice! time: #, wave: 9", time);
                                SetTime(time + 90, 9);
                                Card(HP_33, row + 1, col + 1);
                                Card(M_HMG_15, row + 1, col + 1);
                                ShowError("MNuclear! time: #, wave: 9", time + 90);
                                potPadder.goOn();
                                SetTime(1054 - 100 - 751, 10);
                                potPadder.pause();

                                ++realWave;
                                ++nextPlant;

                                while (!suggestedPositionM.empty()) {
                                    suggestedPositionM.pop();
                                }
                                while (!suggestedPositionN.empty()) {
                                    suggestedPositionN.pop();
                                }
                                suggestedPositionM.push({1, 7});
                                suggestedPositionM.push({3, 8});
                                suggestedPositionM.push({3, 6});
                                suggestedPositionN.push({2, 6});
                                suggestedPositionN.push({1, 5});
                                suggestedPositionN.push({3, 5});
                                suggestedPositionN.push({4, 4});
                                return;
                            }
                        }
                        if (gridInfo[row][col] == -3) {
                            if (jackFlag > 100) {
                                CardNotInQueue(potIndex + 1, row + 1, 6);
                                CardNotInQueue(jalaIndex + 1, row + 1, 6);
                                ShowError("Jala! time: #, wave: 9", time);
                                return;
                            }
                        }
                    }
                }
                break;
            default:
                break;
            }
        }
        return;
    }

    for (auto wave : {20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}) {
        time = NowTime(wave);
        rowArg = RowMean();
        if (realWave <= wave) {
            if (jalaFlag == 0 && seeds[iceIndex].isUsable()) {
                row = suggestedPositionM.front().row;
                col = suggestedPositionM.front().col;
                // time >= 5000 - 1600 - 1254 - 200 - 420 - 90 - 4 * cherryWindow
                if (IsUsableIn(mnuclearIndex, 90)
                        && IsUsableIn(potIndex, 90)
                        && RuinIsUsableIn(row, col, 90)
                    || iceFlag == -2) {
                    if (IsUsableIn(nuclearIndex, 90 + 420 + 200 + 1054 - 100)
                            && IsUsableIn(cherryIndex, 90 + 420 + 200 + 1054 + 200 + 1555 - 100)
                        || suggestedPositionN.front().col <= 5
                            && IsUsableIn(nuclearIndex, 90 + 420 + 200 + 1555 - 100)
                        || iceFlag == -2) {
                        if (jackFlag == 111) {
                            JudgeIce();
                            jalaFlag = -1;
                            if (iceFlag != -2) {
                                iceFlag = 90;
                            } else {
                                iceFlag = -1;
                            }
                            ShowErrorNotInQueue("Ice! time: #, wave: #", time, wave);
                            return;
                        }
                    }
                }
            }
            if (iceFlag == 0 && seeds[mnuclearIndex].isUsable()) {
                if (jackFlag == 111) {
                    row = suggestedPositionM.front().row;
                    col = suggestedPositionM.front().col;
                    CardNotInQueue(potIndex + 1, row + 1, col + 1);
                    CardNotInQueue(mnuclearIndex + 1, row + 1, col + 1);
                    iceFlag = -1;
                    ShowErrorNotInQueue("MNuclear! time: #, wave: #", time, wave);
                    SetNowTime();
                    potPadder.goOn();
                    suggestedPositionM.pop();
                    ++nextPlant;
                    return;
                }
            }
        }
        if (time > 0 && rowArg < 800.0f && realWave < wave) {
            switch (nextPlant) {
            case -1:
                if (time >= 318 - 100) {
                    CardNotInQueue(cherryIndex + 1, 3, 9);
                    SetTime(time + 100, 1);
                    Shovel(3, 9);
                    realWave = wave;
                    ++nextPlant;
                    SetTime(1947 - 420 - 751 - 751, 2);
                    potPadder.pause();
                }
            case 0:
                if (time >= 226 && seeds[jalaIndex].isUsable()) {
                    row = suggestedPositionM.front().row;
                    if (gridInfo[row][5] > -2) {
                        col = 5;
                    } else {
                        if (gridInfo[row][6] > -2) {
                            col = 6;
                        } else {
                            if (gridInfo[row][7] > -2) {
                                col = 7;
                            } else {
                                col = 4;
                                // row = (row == 3) ? 1 : 3;
                                // if (gridInfo[row][6] > -2) {
                                //     col = 6;
                                // } else {
                                //     col = 4;
                                // }
                                // suggestedPositionM.front().row = 1;
                                // suggestedPositionM.front().col = 6;
                                // suggestedPositionN.front().row = 4;
                                // suggestedPositionN.front().col = 4;
                            }
                        }
                    }
                    if (gridInfo[row][col] % 8 == 0
                        || (gridInfo[row][col] == -1 && seeds[potIndex].isUsable())
                        || col == 4) {
                        if (IsUsableIn(mnuclearIndex, 751)
                            && IsUsableIn(nuclearIndex, 751 + 420 + 200 + 1054 - 100)) {
                            if (jackFlag > 100) {
                                if (col == 4) {
                                    ShovelNotInQueue(row + 1, col + 1);
                                }
                                CardNotInQueue(potIndex + 1, row + 1, col + 1);
                                CardNotInQueue(jalaIndex + 1, row + 1, col + 1);
                                jalaFlag = 100;
                                ShowErrorNotInQueue("Jala! time: #, wave: #", time, wave);
                                realWave = wave;
                                // ++nextPlant;
                                if (wave != 9 && wave != 20) {
                                    SetTime(1054 - 100 - 751, wave + 1);
                                    potPadder.pause();
                                }
                                return;
                            }
                        }
                    }
                }
                break;
            case 1:
                if (seeds[nuclearIndex].isUsable()) {
                    if (time >= 1054 - 50 - 0.5 * cherryWindow) {
                        row = suggestedPositionN.front().row;
                        col = suggestedPositionN.front().col;
                        if (wave == 20) {
                            row = 2;
                            col = 6;
                        }
                        if (gridInfo[row][col] < -2) {
                            for (int i = 8; i > 3; --i) {
                                if (gridInfo[0][i] == -1) {
                                    row = 0;
                                    col = i;
                                    break;
                                }
                            }
                            if (gridInfo[row][col] < -2) {
                                for (int i = 8; i > 3; --i) {
                                    if (gridInfo[4][i] == -1) {
                                        row = 4;
                                        col = i;
                                        break;
                                    }
                                }
                            }
                        }
                        if (col == 4 && time < 1555 - 100) {
                            return;
                        }
                        if ((gridInfo[row][col] % 8 == 0 || (gridInfo[row][col] == -1 && seeds[potIndex].isUsable()))) {
                            if (IsUsableIn(cherryIndex, 50 + 200 + 1455)) {
                                if (jackFlag > 100) {
                                    CardNotInQueue(potIndex + 1, row + 1, col + 1);
                                    CardNotInQueue(nuclearIndex + 1, row + 1, col + 1);
                                    ShowErrorNotInQueue("Nuclear! time: #, wave: #", time, wave);
                                    SetNowTime();
                                    potPadder.goOn();
                                    suggestedPositionN.pop();
                                    realWave = wave;
                                    ++nextPlant;
                                    if (wave != 9 && wave != 20) {
                                        SetTime(1555 - 100 - 751, wave + 1);
                                        potPadder.pause();
                                    }
                                    return;
                                }
                            }
                        }
                        return;
                    }
                }
                break;
            case 2:
                if (seeds[cherryIndex].isUsable()) {
                    if (time >= 1054
                        && (gridInfo[2][5] % 8 == 0 || (gridInfo[2][5] == -1 && seeds[potIndex].isUsable()))
                        && IsUsableIn(jalaIndex, 50 + 200 + 1947 - 420 - 751)
                        && IsUsableIn(mnuclearIndex, 50 + 200 + 1947 - 420)
                        && IsUsableIn(nuclearIndex, 50 + 200 + 1947 + 200 + 1054 - 100)) {
                        if (jackFlag > 100) {
                            CardNotInQueue(potIndex + 1, 3, 6);
                            CardNotInQueue(cherryIndex + 1, 3, 6);
                            ShowErrorNotInQueue("Cherry! time: #, wave: #", time, wave);
                            SetNowTime();
                            potPadder.goOn();
                            realWave = wave;
                            nextPlant = 0;
                            if (wave != 9 && wave != 20) {
                                SetTime(1947 - 420 - 751 - 751, wave + 1);
                                potPadder.pause();
                            }
                            return;
                        }
                    }
                    if (wave == 20) {
                        if (seeds[potIndex].isUsable() && gridInfo[2][8] == -1) {
                            if (jackFlag > 100) {
                                CardNotInQueue(potIndex + 1, 3, 9);
                                CardNotInQueue(cherryIndex + 1, 3, 9);
                                ShowErrorNotInQueue("Cherry! time: #, wave: #", time, wave);
                                SetNowTime();
                                potPadder.goOn();
                                realWave = wave;
                                nextPlant = 0;
                                return;
                            }
                        }
                    }
                }
                break;
            default:
                break;
            }
            if (realWave <= wave - 3) {
                ++nextPlant;
                realWave = wave - 1;
            }
            return; //
        }
    }
}

void JudgeClover()
{
    int row = 0, col = 0, i;
    // for (i = 4; i >= 0; --i) {
    //     row = targetPlace[i][0];
    //     col = targetPlace[i][1];
    //     if (gridInfo[row][col] == 0 || gridInfo[row][col] == 8) {
    //         if (jackFlag > 50) {
    //             if (row == 0 || row == 4) {
    //                 if (safeRow[row] > 361.0f + safeWindow - 20.0F) {
    //                     CardNotInQueue(cloverIndex + 1, row + 1, col + 1);
    //                     ShowErrorNotInQueue("Clover!");
    //                     return;
    //                 }
    //             } else {
    //                 if (safeRow[row] > 521.0f + safeWindow - 20.0F) {
    //                     CardNotInQueue(cloverIndex + 1, row + 1, col + 1);
    //                     ShowErrorNotInQueue("Clover!");
    //                     return;
    //                 }
    //             }
    //         }
    //     }
    // }
    for (row = 4; row >= 0; --row) {
        for (col = 5; col >= 0; --col) {
            if (gridInfo[row][col] % 8 == 0) {
                if (jackFlag > 50) {
                    float safeDist = 361.0F + (col - 3) * 80.0F + safeWindow - 20.0F;
                    if (safeRow[row] > safeDist) {
                        CardNotInQueue(cloverIndex + 1, row + 1, col + 1);
                        ShowErrorNotInQueue("Clover!");
                        return;
                    }
                }
            }
        }
    }
    for (i = 0; i < 5; ++i) {
        row = targetPlace[i][0];
        col = targetPlace[i][1];
        if (gridInfo[row][col] > 0) {
            if (GetPlantIndex(row + 1, col + 1, DPG_10) >= 0) {
                ShovelNotInQueue(row + 1, col + 1);
                CardNotInQueue(cloverIndex + 1, row + 1, col + 1);
                ShowErrorNotInQueue("Clover!");
                return;
            }
        }
    }
    if (gridInfo[1][3] % 8 > 0) {
        if (GetPlantIndex(2, 4, DPG_10) >= 0) {
            ShovelNotInQueue(2, 4);
            CardNotInQueue(cloverIndex + 1, 2, 4);
            ShowErrorNotInQueue("Clover!");
            return;
        }
    }
    if (gridInfo[3][3] % 8 > 0) {
        if (GetPlantIndex(4, 4, DPG_10) >= 0) {
            ShovelNotInQueue(4, 4);
            CardNotInQueue(cloverIndex + 1, 4, 4);
            ShowErrorNotInQueue("Clover!");
            return;
        }
    }
    auto seeds = GetMainObject()->seedArray();
    if (seeds[potIndex].isUsable()) {
        for (row = 4; row >= 0; --row) {
            for (col = 8; col >= 0; --col) {
                if (gridInfo[row][col] == -1) {
                    if (jackFlag > 50) {
                        float safeDist = 361.0F + (col - 3) * 80.0F + safeWindow - 20.0F;
                        if (safeRow[row] > safeDist) {
                            CardNotInQueue(potIndex + 1, row + 1, col + 1);
                            CardNotInQueue(cloverIndex + 1, row + 1, col + 1);
                            ShowErrorNotInQueue("Clover!");
                            return;
                        }
                    }
                }
            }
        }
    }
}

void UseClover()
{
    auto _clover = GetMainObject()->seedArray() + GetSeedIndex(SYC_27);
    auto zombie = GetMainObject()->zombieArray();
    int zombieCount = GetMainObject()->zombieTotal();

    if (_clover->isUsable()) {
        for (int index = 0; index < zombieCount; ++index) {
            if (zombie[index].type() == QQ_16
                && !zombie[index].isDead()
                && zombie[index].isExist()
                && !zombie[index].isDisappeared()
                && zombie[index].abscissa() <= 0 * 80) { // original 2.5
                JudgeClover();
                break;
            }
        }
    }

    auto plants = GetMainObject()->plantArray();
    auto plantTotal = GetMainObject()->plantTotal();
    for (int i = 0; i < plantTotal; ++i) {
        if (plants[i].type() == SYC_27
            && plants[i].hp() > 0
            && !plants[i].isCrushed()
            && !plants[i].isDisappeared()
            && plants[i].explodeCountdown() <= 0) {
            ShovelNotInQueue(plants[i].row() + 1, plants[i].col() + 1);
        }
    }
}

void RemovePumpkin(int time, int row, int col)
{
    SetTime(time, 1);
    InsertOperation([=]() {
        if (gridInfo[row - 1][col - 1] > 7) {
            ShovelNotInQueue(row, col, true);
            gridInfo[row - 1][col - 1] -= 8;
        }
    });
}

void PumpkinSet()
{
    int row = 0, col = 0, i;
    // 补内部南瓜1
    for (row = 0; row < 5; ++row) {
        for (col = 0; col < 3; ++col) {
            if (gridInfo[row][col] < 8 && gridInfo[row][col] >= 0) {
                CardNotInQueue(pumpkinIndex + 1, row + 1, col + 1);
                return;
            }
        }
    }
    // 补内部南瓜2
    for (row = 1; row < 4; ++row) {
        if (gridInfo[row][3] < 8 && gridInfo[row][3] >= 0) {
            CardNotInQueue(pumpkinIndex + 1, row + 1, 4);
            return;
        }
    }
    // 补前排南瓜
    if (levelMode >> 1 != 0b10 || NowTime(19) > 2226) {
        for (i = 0; i < 5; ++i) {
            row = critPlace[i][0];
            col = critPlace[i][1];
            if (gridInfo[row][col] < 8 && gridInfo[row][col] >= 0) {
                if (row == 0 || row == 4) {
                    if (safeRow[row] > 391.0f + safeWindow) {
                        CardNotInQueue(pumpkinIndex + 1, row + 1, col + 1);
                        return;
                    }
                } else {
                    if (safeRow[row] > 471.0f + safeWindow) {
                        CardNotInQueue(pumpkinIndex + 1, row + 1, col + 1);
                        return;
                    }
                }
            }
        }
    }
    // 修南瓜
    auto plants = GetMainObject()->plantArray();
    auto plantTotal = GetMainObject()->plantTotal();
    for (i = 0; i < plantTotal; ++i) {
        if (plants[i].type() == NGT_30
            && plants[i].hp() > 0
            && plants[i].hp() < 1500
            && !plants[i].isCrushed()
            && !plants[i].isDisappeared()) {
            CardNotInQueue(pumpkinIndex + 1, plants[i].row() + 1, plants[i].col() + 1);
        }
    }
}

void PumpkinPad()
{
    auto seed = GetMainObject()->seedArray();
    SafePtr<Zombie_w_No> zombie = (Zombie_w_No*)(GetMainObject()->zombieArray());

    int index = 0, zombie_count_max = 0, zombie_type = 0, zombie_row = 0;
    float zombie_abscissa = 0.0;

    zombie_count_max = GetMainObject()->zombieTotal();
    for (index = 0; index < zombie_count_max; ++index) {
        if (zombie[index].isExist() && !zombie[index].isDead()) {
            if (zombie[index].type() == BY_23 || zombie[index].type() == HY_32) {
                zombie_abscissa = zombie[index].abscissa();
                zombie_row = zombie[index].row() + 1;
                if (zombie[index].isHammering()) {
                    if (FindAnimation(zombie[index].animNo()) > 0.64f) {
                        int targetCol = ((int)zombie_abscissa + 8) / 80;
                        if (zombie_row == 1 || zombie_row == 5) {
                            if (zombie_abscissa <= 391.0f) {
                                RemovePumpkin(NowTime(1), zombie_row, targetCol);
                            }
                        } else {
                            if (zombie_abscissa <= 471.0f) {
                                RemovePumpkin(NowTime(1), zombie_row, targetCol);
                            }
                        }
                    }
                }
            }
            if (zombie[index].type() == BC_12
                && zombie[index].hp() > 0
                && zombie[index].isExist()
                && !zombie[index].isDead()
                && !zombie[index].isDisappeared()) {
                zombie_abscissa = zombie[index].abscissa();
                zombie_row = zombie[index].row() + 1;
                int targetCol = ((int)zombie_abscissa + 48 - 1) / 80;
                if (zombie_row == 1 || zombie_row == 5) {
                    if (zombie_abscissa < 351.0f + 1.0f) {
                        RemovePumpkin(NowTime(1), zombie_row, targetCol);
                    }
                } else {
                    if (zombie_abscissa < 431.0f + 1.0f) {
                        RemovePumpkin(NowTime(1), zombie_row, targetCol);
                    }
                }
            }
        }
    }
    if (seed[GetSeedIndex(NGT_30)].isUsable()) {
        PumpkinSet();
    }
}

void PotPad()
{
    auto seed = GetMainObject()->seedArray();
    auto zombie = GetMainObject()->zombieArray();
    // DrawGrid();

    if (seed[potIndex].isUsable()) {
        if (levelMode >= 0b01000) {
            // action 0: begin wave
            if (NowTime(1) < 318 && gridInfo[2][8] == -1) {
                SafePlacePot(3, 9);
                // ShowErrorNotInQueue("Pot Begin! row: 3, col: 9");
                return;
            }
        }

        int row, col, i;

        // action 1: fix
        for (i = 0; i < 5; ++i) {
            row = critPlace[i][0];
            col = critPlace[i][1];
            if (gridInfo[row][col] == -1) {
                SafePlacePot(row + 1, col + 1);
                // ShowErrorNotInQueue("Pot Fix! row: #, col: #", row + 1, col + 1);
                return;
            }
        }
        for (row = 0; row < 5; ++row) {
            for (col = 0; col < 4; ++col) {
                if (gridInfo[row][col] == -1) {
                    SafePlacePot(row + 1, col + 1);
                    // ShowErrorNotInQueue("Pot Fix! row: #, col: #", row + 1, col + 1);
                    return;
                }
            }
        }

        if (levelMode >= 0b01000) {
            // action 2: pad
            int rows[5] = {-1, -1, -1, -1, -1};
            float abscissas[5] = {800.0f, 800.0f, 800.0f, 800.0f, 800.0f};
            for (i = 0; i < 5; ++i) {
                float abscissa = safeRowG[i];
                if (i == 0 || i == 4) {
                    abscissa += 80.0f;
                }
                for (int j = 0; j < 5; ++j) {
                    if (abscissa <= abscissas[j]) {
                        for (int k = 4; k > j; --k) {
                            abscissas[k] = abscissas[k - 1];
                            rows[k] = rows[k - 1];
                        }
                        abscissas[j] = abscissa;
                        rows[j] = i;
                    }
                }
            }
            for (i = 0; i < 5; ++i) {
                if (abscissas[i] <= 521.0f) {
                    row = rows[i];
                    float abscissa = abscissas[i];
                    if (row == 0 || row == 4) {
                        abscissa -= 80.0f;
                    }
                    col = ((int)abscissa - 42) / 80;
                    if (gridInfo[row][col] == -1) {
                        SafePlacePot(row + 1, col + 1);
                        // ShowErrorNotInQueue("Pot Pad! row: #, col: #", row + 1, col + 1);
                        return;
                    }
                }
            }

            // action 3: prepare
            for (i = 0; i < 3; ++i) {
                row = targetPlace[i][0];
                col = targetPlace[i][1];
                if (gridInfo[row][col] == -1
                    && safeRow[row] > 521.0f) {
                    SafePlacePot(row + 1, col + 1);
                    // ShowErrorNotInQueue("Pot Prepare! row: #, col: #", row + 1, col + 1);
                    return;
                }
            }
        }
    }
}

bool JudgeImp(int row)
{
    auto zombies = GetMainObject()->zombieArray();
    int zombieCount = GetMainObject()->zombieTotal();
    for (int i = 0; i < zombieCount; ++i) {
        if (zombies[i].type() == XG_24
            && zombies[i].row() == row
            && zombies[i].hp() > 0
            && !zombies[i].isDead()
            && zombies[i].isExist()
            && !zombies[i].isDisappeared()) {
            if (zombies[i].abscissa() > 310.0f) {
                return false;
            }
        }
    }
    return true;
}

void GshroomFix()
{
    auto plants = GetMainObject()->plantArray();
    auto plantTotal = GetMainObject()->plantTotal();
    auto fshroom = GetMainObject()->seedArray() + fshroomIndex;
    auto gshroom = GetMainObject()->seedArray() + gshroomIndex;

    int row = 0, col = 0, i;
    if (fshroom->isUsable()) {
        if (gridInfo[1][3] % 8 == 0) {
            CardNotInQueue(fshroomIndex + 1, 2, 4);
        }
        if (gridInfo[3][3] % 8 == 0) {
            CardNotInQueue(fshroomIndex + 1, 4, 4);
        }
        for (i = 0; i < 5; ++i) {
            row = critPlace[i][0];
            col = critPlace[i][1];
            if (gridInfo[row][col] % 8 == 0) {
                if (row == 0 || row == 4) {
                    if (safeRow[row] > 361.0f) {
                        CardNotInQueue(fshroomIndex + 1, row + 1, col + 1);
                        break;
                    }
                } else {
                    if (safeRow[row] > 441.0f) {
                        CardNotInQueue(fshroomIndex + 1, row + 1, col + 1);
                        break;
                    }
                }
            }
        }
    }
    if (gshroom->isUsable()) {
        if (levelMode >> 1 != 0b10 || NowTime(19) > 2226) {
            if (GetPlantIndex(2, 4, DPG_10) >= 0) {
                CardNotInQueue(gshroomIndex + 1, 2, 4);
            }
            if (GetPlantIndex(4, 4, DPG_10) >= 0) {
                CardNotInQueue(gshroomIndex + 1, 4, 4);
            }
        }
        for (i = 0; i < 3; ++i) {
            row = critPlace[i][0];
            col = critPlace[i][1];
            if (GetPlantIndex(row + 1, col + 1, DPG_10) >= 0
                && safeRow[row] > 441.0f + safeWindow) {
                if (gridInfo[row][col] == 8 || JudgeImp(row)) {
                    CardNotInQueue(gshroomIndex + 1, row + 1, col + 1);
                    break;
                }
            }
        }
    }
}

void SunflowerFix()
{
    auto seeds = GetMainObject()->seedArray();
    int row, col, i;
    if (sunflowerIndex != -1 && seeds[sunflowerIndex].isUsable()) {
        for (i = 0; i < 7; ++i) {
            row = flowerPlace[i][0];
            col = flowerPlace[i][1];
            if (gridInfo[row][col] % 8 == 0
                && safeRow[row] > 361.0F) {
                CardNotInQueue(sunflowerIndex + 1, row + 1, col + 1);
                break;
            }
        }
    }
    if (twinflowerIndex != -1 && seeds[twinflowerIndex].isUsable()) {
        for (i = 0; i < 7; ++i) {
            row = flowerPlace[i][0];
            col = flowerPlace[i][1];
            if (GetPlantIndex(row + 1, col + 1, XRK_1) >= 0
                && gridInfo[row][col] == 9
                && safeRow[row] > 441.0F + safeWindow) {
                CardNotInQueue(twinflowerIndex + 1, row + 1, col + 1);
                break;
            }
        }
    }
}

void slowPlay()
{
    AINManeger.pushFunc(AIN);
    NManager.pushFunc(Nuclear);
    AManager.pushFunc(Cherry);

    // wave 1
    // Card(HP_33, 3, 9);
    SetTime(318 - 100, 1);
    Card(YTZD_2, 3, 9);
    SetTime(318, 1);
    Shovel(3, 9);

    // pot control
    for (auto wave : {2, 5, 8, 13, 16, 19}) {
        SetTime(1947 - 420 - 751 - 751, wave);
        potPadder.pause();
    }
    for (auto wave : {3, 6, 9, 17}) {
        SetTime(1054 - 100 - 751, wave);
        potPadder.pause();
    }
    for (auto wave : {4, 7, 11, 12, 14, 15, 18}) {
        SetTime(1555 - 100 - 751, wave);
        potPadder.pause();
    }

    // wave 10
    SetTime(-725 + 751, 10);
    Card(HP_33, 2, 8);
    Card(M_HMG_15, 2, 8);
    potPadder.goOn();
    SetTime(-725, 10);
    potPadder.pause();

    // wave 20
    SetTime(-600, 20);
    InsertOperation([=]() {
        nuclearFlag = false;
    });
    SetTime(394 - 100 - 751, 20);
    potPadder.pause();
}

void changePlay()
{
    nextPlant = -1;
    realWave = 0;
    suggestedPositionN = std::queue<Position>();
    suggestedPositionM = std::queue<Position>();

    suggestedPositionM.push({1, 8});
    suggestedPositionM.push({3, 7});
    suggestedPositionM.push({2, 7});

    suggestedPositionN.push({1, 6});
    suggestedPositionN.push({3, 6});
    suggestedPositionN.push({2, 6});

    C3Player.pushFunc(C3change);
    return;
}

void midPlay()
{
    jalaPlace[4][0] = 1;
    jalaPlace[5][0] = 3;
    C3Player.pushFunc(C3sharp);

    // wave 1
    SetTime(318 - 100, 1);
    Card(YTZD_2, 3, 9);
    SetTime(318, 1);
    Shovel(3, 9);

    // pot control
    for (auto wave : {2, 5, 8, 11, 14, 17}) {
        SetTime(1947 - 420 - 751 - 751, wave);
        potPadder.pause();
    }
    for (auto wave : {3, 6, 9, 12, 15, 18}) {
        SetTime(1054 - 100 - 751, wave);
        potPadder.pause();
    }
    for (auto wave : {4, 7, 10, 13, 16, 19}) {
        SetTime(1555 - 100 - 751, wave);
        potPadder.pause();
    }
    SetTime(394 - 100 + 1 + 90 - 751, 20);
    potPadder.pause();

    // jala control
    for (auto wave : {17, 14, 11, 8, 5, 2}) {
        SetTime(0, wave);
        InsertOperation([=]() {
            jalaBlocker = false;
        });
    }
}

void Script()
{
    SetErrorMode(CONSOLE);
    OpenMultipleEffective('C', MAIN_UI_OR_FIGHT_UI);
    SetGameSpeed(2);
    SetLevelMode();
    SelectLevelCards();

    memset(gridInfo, -1, sizeof(gridInfo));
    memset(crushedPot, 0, sizeof(crushedPot));
    gridViewer.pushFunc(DrawGrid);

    if (levelMode & 0b1) {
        cloverUser.pushFunc(UseClover);
    }
    pumpkinPadder.pushFunc(PumpkinPad);
    potPadder.pushFunc(PotPad);
    gshroomFixer.pushFunc(GshroomFix);
    sunflowerFixer.pushFunc(SunflowerFix);

    jalaPlace[4][0] = 3;
    jalaPlace[4][1] = 6;
    jalaPlace[5][0] = 2;
    mnuclearPlace[4][0] = 3;
    mnuclearPlace[4][1] = 7;
    nuclearPlace[5][0] = 3;
    nuclearPlace[5][1] = 6;

    SetTime(-600, 1);
    GetIndex();

    int level_mode = levelMode >> 1;
    switch (level_mode) {
        // 慢速关
    case 0b1100:
    case 0b1101:
    case 0b1110:
    case 0b1111:
        slowPlay();
        break;
        // 变速关
    case 0b1000:
    case 0b1001:
    case 0b1010:
    case 0b1011:
        changePlay();
        break;
        // 中速关
    case 0b0100:
    case 0b0101:
    case 0b0110:
    case 0b0111:
        midPlay();
        break;
        // 快速关
    case 0b0011:
        jalaPlace[4][1] = 5;
        C3Player.pushFunc(C3plus);
        for (auto wave : {20, 17, 14, 11, 8, 5, 2}) {
            SetTime(1352 - 420 - 751, wave);
            potPadder.pause();
        }
        for (auto wave : {19, 16, 13, 10, 7, 4, 1}) {
            SetTime(1352 - 100 - 751, wave);
            potPadder.pause();
        }
        for (auto wave : {18, 15, 12, 9, 6, 3}) {
            SetTime(1947 - 100 - 751, wave);
            potPadder.pause();
        }
        break;
    case 0b0010:
        C3Player.pushFunc(C3minus);
        SetTime(-600, 1);
        Shovel(2, 4);
        Shovel(4, 4);
        SetTime(394 - 100 + 1, 10);
        InsertOperation([=]() {
            JudgeIce();
        });
        SetTime(394 - 100 + 1, 20);
        InsertOperation([=]() {
            JudgeIce();
        });
        for (auto wave : {18, 15, 12, 8, 5, 2}) {
            SetTime(1555 - 100 - 751, wave);
            potPadder.pause();
        }
        SetTime(394 - 100 + 1 - 751, 20);
        potPadder.pause();
        break;
    case 0b0001:
        C3Player.pushFunc(C3);
        break;
    default:
        SetTime(394 - 100 + 1, 10);
        InsertOperation([=]() {
            JudgeIce();
        });
        SetTime(394 - 100 + 1, 20);
        InsertOperation([=]() {
            JudgeIce();
        });
        break;
    }
}
