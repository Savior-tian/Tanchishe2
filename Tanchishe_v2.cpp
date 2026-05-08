#include<stdio.h>
#include<time.h>
#include<windows.h>
#include<stdlib.h>
#include<string.h>
#include<conio.h>

#define U 1
#define D 2
#define L 3
#define R 4       // 蛇的状态：U:上 D:下 L:左 R:右

// 用户相关宏定义
#define MAX_USERNAME 32
#define MAX_PASSWORD 32
#define USER_FILE    "users.dat"
#define LOG_FILE     "gamelog.dat"
#define OX           12          // 游戏地图水平偏移量，使整体布局在110列控制台中居中

typedef struct SNAKE // 蛇的每一个节点
{
    int x;
    int y;
    struct SNAKE* next;
}snake;

// 全局变量 //
int score = 0, add = 10;        // 总得分，每次吃食物得分
int status, sleeptime = 200;    // 每次运行的时间间隔
snake* head, * food;            // 蛇头指针，食物指针
snake* q;                       // 遍历蛇体时用到的指针
int endgamestatus = 0;          // 游戏结束状态：1撞墙 2咬到自己 3主动退出游戏
int afterGameChoice = 0;        // 游戏结束后的选择：1=再来一把 0=回到主界面
char currentUser[MAX_USERNAME] = ""; // 当前登录用户名
int  currentUserID = 0;              // 当前登录用户ID
time_t gameStartTime = 0;            // 本局游戏开始时间

// 声明全部函数 //
void Pos(int x, int y);
void creatMap();
void initsnake();
int  biteself();
void createfood();
void cantcrosswall();
void snakemove();
void pause();
void gamecircle();
void welcometogame();
void endgame();
void gamestart();
// 新增功能函数声明
void SetColor(int color);
void HideCursor();
void ShowCursor();
void drawInfoPanel();
int  userExists(const char* username);
int  registerUser();
int  loginUser();
void saveGameLog(int endStatus, int finalScore);
void showGameLog();
void readPassword(char* buf, int maxLen);
void showMainMenu();
int  strDisplayWidth(const char* s);
void printPadded(const char* str, int displayW);

void Pos(int x, int y) // 设置光标位置
{
    COORD pos;
    HANDLE hOutput;
    pos.X = x;
    pos.Y = y;
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(hOutput, pos);
}

void SetColor(int color) // 设置控制台文字颜色
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void HideCursor() // 隐藏控制台光标
{
    CONSOLE_CURSOR_INFO cursor;
    cursor.bVisible = FALSE;
    cursor.dwSize = 1;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);
}

void ShowCursor() // 显示控制台光标
{
    CONSOLE_CURSOR_INFO cursor;
    cursor.bVisible = TRUE;
    cursor.dwSize = 20;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);
}

void creatMap() // 创建地图
{
    int i;
    SetColor(14); // 黄色边框
    for (i = 0; i < 58; i += 2) // 打印上下边框
    {
        Pos(OX + i, 0);
        printf("■");
        Pos(OX + i, 26);
        printf("■");
    }
    for (i = 1; i < 26; i++) // 打印左右边框
    {
        Pos(OX, i);
        printf("■");
        Pos(OX + 56, i);
        printf("■");
    }
    SetColor(7); // 恢复默认颜色
}

void initsnake() // 初始化蛇体
{
    snake* tail;
    int i;
    tail = (snake*)malloc(sizeof(snake));
    tail->x = OX + 24;
    tail->y = 5;
    tail->next = NULL;
    for (i = 1; i <= 4; i++)
    {
        head = (snake*)malloc(sizeof(snake));
        head->next = tail;
        head->x = OX + 24 + 2 * i;
        head->y = 5;
        tail = head;
    }
    SetColor(10); // 绿色蛇体
    while (tail != NULL)
    {
        Pos(tail->x, tail->y);
        printf("■");
        tail = tail->next;
    }
    SetColor(7);
}

int biteself() // 判断是否咬到了自己
{
    snake* self;
    self = head->next;
    while (self != NULL)
    {
        if (self->x == head->x && self->y == head->y)
            return 1;
        self = self->next;
    }
    return 0;
}

void createfood() // 随机创建食物
{
    snake* food_1;
    food_1 = (snake*)malloc(sizeof(snake));
    // 确保x坐标为偶数（2,4,...,52），y在边界内
    do {
        food_1->x = OX + (rand() % 26 + 1) * 2;
        food_1->y = rand() % 24 + 1;
        // 检查是否与蛇体重合
        int overlap = 0;
        q = head;
        while (q != NULL)
        {
            if (q->x == food_1->x && q->y == food_1->y)
            {
                overlap = 1;
                break;
            }
            q = q->next;
        }
        if (!overlap) break;
    } while (1);
    Pos(food_1->x, food_1->y);
    food = food_1;
    SetColor(12); // 红色食物
    printf("★");
    SetColor(7);
}

void cantcrosswall() // 不能穿墙
{
    if (head->x == OX || head->x == OX + 56 || head->y == 0 || head->y == 26)
    {
        endgamestatus = 1;
        endgame();
    }
}

void snakemove() // 蛇前进：上U 下D 左L 右R
{
    snake* nexthead;
    cantcrosswall();
    if (endgamestatus) return; // 撞墙后直接返回，不继续移动

    // 根据方向计算偏移量（x方向步长为2，因为每个字符占2列）
    int dx = 0, dy = 0;
    if      (status == U) { dx =  0; dy = -1; }
    else if (status == D) { dx =  0; dy =  1; }
    else if (status == L) { dx = -2; dy =  0; }
    else if (status == R) { dx =  2; dy =  0; }
    else return;

    nexthead = (snake*)malloc(sizeof(snake));
    nexthead->x = head->x + dx;
    nexthead->y = head->y + dy;

    if (nexthead->x == food->x && nexthead->y == food->y)
    {
        // 吃到食物：蛇体增长，重绘整条蛇
        nexthead->next = head;
        head = nexthead;
        SetColor(10); // 绿色蛇体
        q = head;
        while (q != NULL) { Pos(q->x, q->y); printf("■"); q = q->next; }
        SetColor(7);
        score = score + add;
        createfood();
    }
    else
    {
        // 正常移动：头部前进，擦除尾部
        nexthead->next = head;
        head = nexthead;
        q = head;
        SetColor(10); // 绿色蛇体
        while (q->next->next != NULL) { Pos(q->x, q->y); printf("■"); q = q->next; }
        SetColor(7);
        Pos(q->next->x, q->next->y);
        printf("  ");
        free(q->next);
        q->next = NULL;
    }

    if (biteself() == 1)
    {
        endgamestatus = 2;
        endgame();
        return; // 咬到自己后直接返回
    }
}

void drawInfoPanel() // 绘制右侧信息面板，垂直居中于游戏区域(y=0~26)
{
    // 面板区域：y=7~18（12行），居中于27行游戏区；x=62，居中于右侧区域(x=58~99)
    int px = OX + 62;  // 信息面板起始列（地图右壁OX+56吸后加6列间距）

    SetColor(14); // 黄色标题
    Pos(px, 7);  printf("--[ \u6e38\u620f\u4fe1\u606f ]--");
    Pos(px, 8);  printf("------------------");

    SetColor(11); // 青色得分
    Pos(px, 9);  printf(" \u5f97\u5206\uff1a%-6d    ", score);
    Pos(px, 10); printf(" \u98df\u7269\u5f97\u5206\uff1a%-4d\u5206", add);

    SetColor(7);
    Pos(px, 11); printf("------------------");

    Pos(px, 12); printf(" 不能穿墙，不能咬到自己");
    Pos(px, 13); printf(" ↑↓←→ 控制移动");
    Pos(px, 14); printf(" F1加速  F2减速");
    Pos(px, 15); printf(" Space暂停 ESC退出");
    Pos(px, 16); printf(" F5 查看游戏日志");

    Pos(px, 17); printf("------------------");

    SetColor(10); // 绿色用户名
    Pos(px, 18); printf(" \u2605 %-12s \u2605", currentUser);
    SetColor(7);
}

void pause() // 暂停
{
    while (1)
    {
        Sleep(300);
        if (GetAsyncKeyState(VK_SPACE))
            break;
    }
}

void gamecircle() // 控制游戏
{
    drawInfoPanel(); // 绘制居中信息面板

    status = R;
    while (1)
    {
        // 仅刷新动态得分，其余静态内容无需每帧重绘
        SetColor(11);
        Pos(OX+62, 9);  printf(" 得分：%-6d    ", score);
        Pos(OX+62, 10); printf(" 食物得分：%-4d分", add);
        SetColor(7);

        if (GetAsyncKeyState(VK_UP) && status != D)
            status = U;
        else if (GetAsyncKeyState(VK_DOWN) && status != U)
            status = D;
        else if (GetAsyncKeyState(VK_LEFT) && status != R)
            status = L;
        else if (GetAsyncKeyState(VK_RIGHT) && status != L)
            status = R;
        else if (GetAsyncKeyState(VK_SPACE))
            pause();
        else if (GetAsyncKeyState(VK_ESCAPE))
        {
            endgamestatus = 3;
            endgame(); // ESC直接在这里处理结束
            break;
        }
        else if (GetAsyncKeyState(VK_F1))
        {
            if (sleeptime >= 50)
            {
                sleeptime -= 30;
                add += 2;
                if (sleeptime == 320) add = 2;
            }
        }
        else if (GetAsyncKeyState(VK_F2))
        {
            if (sleeptime < 350)
            {
                sleeptime += 30;
                add -= 2;
                if (sleeptime == 350) add = 1;
            }
        }
        else if (GetAsyncKeyState(VK_F5))
        {
            showGameLog();
            // 恢复游戏界面
            system("cls");
            creatMap();
            HideCursor();
            snake* tmp = head;
            SetColor(10);
            while (tmp != NULL) { Pos(tmp->x, tmp->y); printf("■"); tmp = tmp->next; }
            SetColor(12);
            Pos(food->x, food->y); printf("★");
            SetColor(7);
            drawInfoPanel(); // 统一调用，避免重复代码
        }
        Sleep(sleeptime);
        snakemove();
        if (endgamestatus) break; // 死亡时退出循环（endgame已在snakemove内调用）
    }
}

void welcometogame() // 开始界面
{
    Pos(40, 12);
    printf("欢迎来到贪吃蛇游戏！");
    SetColor(11); Pos(40, 14); printf("按任意键继续..."); SetColor(7);
    system("pause>nul");
    system("cls");
    Pos(25, 12);
    printf("↑.↓.←.→分别控制蛇的移动  F1 为加速，F2 为减速");
    Pos(25, 13);
    printf("加速将获得更高的分数！");
    SetColor(11); Pos(25, 15); printf("按任意键继续..."); SetColor(7);
    system("pause>nul");
    system("cls");
}

void endgame() // 结束游戏
{
    saveGameLog(endgamestatus, score);

    // 释放蛇和食物内存
    snake* tmp;
    while (head != NULL) { tmp = head; head = head->next; free(tmp); }
    if (food != NULL) { free(food); food = NULL; }
    head = NULL; q = NULL;

    while (1)
    {
        system("cls");
        ShowCursor(); // 结案后显示光标
        Pos(20, 9);
        if (endgamestatus == 1)
        {
            SetColor(12); // 红色失败提示
            printf("对不起，您撞到墙了！游戏结束！");
        }
        else if (endgamestatus == 2)
        {
            SetColor(12);
            printf("对不起，您咬到自己了！游戏结束！");
        }
        else if (endgamestatus == 3)
        {
            SetColor(11); // 青色退出提示
            printf("您已经退出了游戏。");
        }
        SetColor(7);
        Pos(20, 10);
        SetColor(14); // 黄色得分
        printf("本局得分：%d 分", score);
        SetColor(7);
        Pos(20, 12);
        printf("1. 再来一把");
        Pos(20, 13);
        printf("2. 查看日志");
        Pos(20, 14);
        printf("3. 回到主界面");
        Pos(20, 16);
        SetColor(11);
        printf("请选择（1-3）：");
        SetColor(7);

        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        int ch = 0;
        scanf("%d", &ch);
        while (getchar() != '\n');

        if (ch == 1)
        {
            // 不在这里重置 endgamestatus，让 gamecircle 的 break 正常触发后再重新开始
            afterGameChoice = 1;
            return;
        }
        else if (ch == 2)
        {
            showGameLog();
            // showGameLog 结束后继续循环，重新显示结束菜单
        }
        else if (ch == 3)
        {
            afterGameChoice = 0;
            return;
        }
        // 无效输入：继续循环重新显示菜单
    }
}

// 保存游戏日志
void saveGameLog(int endStatus, int finalScore)
{
    FILE* fp = fopen(LOG_FILE, "a");
    if (fp == NULL) return;

    // 开始时间
    struct tm* st = localtime(&gameStartTime);
    char startStr[32];
    strftime(startStr, sizeof(startStr), "%Y-%m-%d %H:%M:%S", st);

    // 持续时长（秒）
    time_t endTime = time(NULL);
    long duration = (long)(endTime - gameStartTime);
    int dur_min = (int)(duration / 60);
    int dur_sec = (int)(duration % 60);

    fprintf(fp, "%d|%s|%s|%02d:%02d|%d\n",
            currentUserID, currentUser, startStr, dur_min, dur_sec, finalScore);
    fclose(fp);
}

// 计算UTF-8字符串在终端中的显示列宽（CJK占2列，ASCII到1列）
int strDisplayWidth(const char* s)
{
    int w = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if      (c < 0x80) { w += 1; s += 1; }
        else if (c < 0xE0) { w += 1; s += 2; }
        else if (c < 0xF0) { w += 2; s += 3; }
        else               { w += 2; s += 4; }
    }
    return w;
}

// 打印字符串并用空格填充到指定显示列宽
void printPadded(const char* str, int displayW)
{
    printf("%s", str);
    int dw = strDisplayWidth(str);
    for (int i = dw; i < displayW; i++) printf(" ");
}

// 显示游戏日志
void showGameLog()
{
    system("cls");
    ShowCursor();
    // 用 printf 加空格前缀居中，数据行自然换行滚动，不受屏幕行数限制
    // 标题34列居中：(110-34)/2=38 → 38个空格前缀
    // 表格63列居中：(110-63)/2=23 → 23个空格前缀（用宏 PREFIX 表示）
#define LOG_PREFIX "                       "   /* 23个空格 */
#define LOG_TITLE  "                                      "  /* 38个空格 */

    SetColor(14);
    printf("\n\n%s========== 游戏用户日志 ==========\n\n", LOG_TITLE);

    SetColor(11);
    printf("%s", LOG_PREFIX);
    printPadded("ID",       8);
    printPadded("用户名",   18);
    printPadded("开始时间", 23);
    printPadded("时长",     10);
    printf("得分\n");

    SetColor(7);
    printf("%s", LOG_PREFIX);
    printPadded("------",                8);
    printPadded("----------------",     18);
    printPadded("---------------------", 23);
    printPadded("--------",             10);
    printf("------\n");

    FILE* fp = fopen(LOG_FILE, "r");
    if (fp == NULL)
    {
        printf("%s暂无游戏记录。\n", LOG_PREFIX);
    }
    else
    {
        int uid, pts;
        char uname[MAX_USERNAME], startStr[32], dur[16];
        int count = 0;
        while (fscanf(fp, "%d|%31[^|]|%31[^|]|%8[^|]|%d\n",
                      &uid, uname, startStr, dur, &pts) == 5)
        {
            char idStr[12];
            sprintf(idStr, "%d", uid);
            printf("%s", LOG_PREFIX);
            printPadded(idStr,    8);
            printPadded(uname,   18);
            printPadded(startStr, 23);
            printPadded(dur,      10);
            printf("%d\n", pts);
            count++;
        }
        fclose(fp);
        if (count == 0)
            printf("%s暂无游戏记录。\n", LOG_PREFIX);
    }

    printf("\n%s按任意键继续...", LOG_PREFIX);
    system("pause>nul");
}

void gamestart() // 游戏开始前
{
    // 重置游戏变量（包括再来一把的情况）
    score = 0; add = 10; sleeptime = 200; endgamestatus = 0;
    system("cls");
    system("mode con cols=110 lines=30");
    HideCursor(); // 游戏中隐藏光标，界面更美观
    // 刷新输入缓冲，防止主菜单残留按键触发游戏内pause()
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    creatMap();
    initsnake();
    createfood();
    gameStartTime = time(NULL); // 记录游戏开始时间
}

// 隐式读入密码，将输入显示为 *
void readPassword(char* buf, int maxLen)
{
    int i = 0;
    int ch;
    while (i < maxLen - 1)
    {
        ch = _getch();
        if (ch == '\r' || ch == '\n') break; // 回车结束
        if (ch == '\b' || ch == 127)          // 退格
        {
            if (i > 0)
            {
                i--;
                printf("\b \b"); // 擦除屏幕上的 *
            }
            continue;
        }
        if (ch >= 32 && ch <= 126) // 可打印字符
        {
            buf[i++] = (char)ch;
            printf("*");
        }
    }
    buf[i] = '\0';
    printf("\n");
}

// 检查用户名是否已存在（返回1存在，0不存在）
int userExists(const char* username)
{
    FILE* fp = fopen(USER_FILE, "r");
    if (fp == NULL) return 0;
    char uname[MAX_USERNAME], pwd[MAX_PASSWORD];
    int id;
    while (fscanf(fp, "%d %31s %31s", &id, uname, pwd) == 3)
    {
        if (strcmp(uname, username) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

// 用户注册（返回1成功，0失败）
int registerUser()
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char confirm[MAX_PASSWORD];

    system("cls");
    printf("\n\n");
    printf("                                        ========== 用户注册 ==========\n\n");

    while (1)
    {
        printf("                                        请输入用户名（不超过31个字符）: ");
        scanf("%31s", username);
        if (userExists(username))
            printf("                                        该用户名已存在，请换一个。\n");
        else
            break;
    }

    while (1)
    {
        printf("                                        请输入密码（不超过31个字符）: ");
        readPassword(password, MAX_PASSWORD);
        printf("                                        请再次确认密码: ");
        readPassword(confirm, MAX_PASSWORD);
        if (strcmp(password, confirm) == 0)
            break;
        printf("                                        两次密码不一致，请重新输入。\n");
    }

    FILE* fp = fopen(USER_FILE, "a");
    if (fp == NULL)
    {
        printf("                                        注册失败：无法写入用户文件。\n");
        return 0;
    }
    // 生成新用户ID = 当前文件中的用户数 + 1
    int newId = 1;
    FILE* fr = fopen(USER_FILE, "r");
    if (fr != NULL)
    {
        char u[MAX_USERNAME], p[MAX_PASSWORD];
        int id;
        int cnt = 0;
        while (fscanf(fr, "%d %31s %31s", &id, u, p) == 3) cnt++;
        fclose(fr);
        newId = cnt + 1;
    }
    fprintf(fp, "%d %s %s\n", newId, username, password);
    fclose(fp);

    currentUserID = newId;
    printf("\n                                        注册成功！用户名：%s  ID：%d\n", username, newId);
    strncpy(currentUser, username, MAX_USERNAME - 1);
    return 1;
}

// 用户登录（返回1成功，0失败）
int loginUser()
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int attempts = 0;
    const int MAX_ATTEMPTS = 3;

    system("cls");
    printf("\n\n");
    printf("                                        ========== 用户登录 ==========\n\n");

    while (attempts < MAX_ATTEMPTS)
    {
        printf("                                        请输入用户名: ");
        scanf("%31s", username);
        printf("                                        请输入密码: ");
        readPassword(password, MAX_PASSWORD);

        FILE* fp = fopen(USER_FILE, "r");
        if (fp == NULL)
        {
            printf("                                        用户文件不存在，请先注册。\n");
            return 0;
        }
        char uname[MAX_USERNAME], pwd[MAX_PASSWORD];
        int uid = 0;
        int found = 0;
        while (fscanf(fp, "%d %31s %31s", &uid, uname, pwd) == 3)
        {
            if (strcmp(uname, username) == 0 && strcmp(pwd, password) == 0)
            {
                found = 1;
                break;
            }
        }
        fclose(fp);

        if (found)
        {
            currentUserID = uid;
            strncpy(currentUser, username, MAX_USERNAME - 1);
            printf("\n                                        登录成功！欢迎，%s（ID:%d）！\n", username, uid);
            Sleep(1000);
            return 1;
        }
        else
        {
            attempts++;
            if (attempts < MAX_ATTEMPTS)
                printf("                                        用户名或密码错误，还有 %d 次机会。\n\n", MAX_ATTEMPTS - attempts);
        }
    }
    printf("                                        登录失败次数过多，请重试。\n");
    Sleep(1200);
    return 0;
}

// 主菜单
void showMainMenu()
{
    system("cls");
    ShowCursor();
    // 水平居中：110列，分隔线42字符，左起col=34；菜单项col=36
    // 垂直居中：30行，菜单约12行，从第7行开始
    int col = 34, row = 7;

    SetColor(14); // 黄色标题
    Pos(col,     row++); printf("==========================================");
    // 标题"欢迎来到贪吃蛇"显示到4(26列)，在42字符框内居中：(42-26)/2=8
    Pos(col + 8, row++); printf("欢  迎  来  到  贪  吃  蛇");
    Pos(col,     row++); printf("==========================================");
    SetColor(7);
    row++; // 空行

    if (currentUser[0] != '\0')
    {
        SetColor(10); // 绿色已登录用户
        Pos(col + 2, row++); printf("当前登录：%s (ID:%d)", currentUser, currentUserID);
        SetColor(7);
    }
    else
    {
        SetColor(8); // 灰色未登录状态
        Pos(col + 2, row++); printf("当前状态：未登录");
        SetColor(7);
    }
    row++; // 空行

    Pos(col + 2, row++); printf("1. 注册账户");
    Pos(col + 2, row++); printf("2. 登录");
    Pos(col + 2, row++); printf("3. 进入游戏");
    Pos(col + 2, row++); printf("4. 查看游戏日志");
    Pos(col + 2, row++); printf("5. 退出");
    row++; // 空行

    SetColor(11); // 青色输入提示
    Pos(col + 2, row);    printf("请选择 [1-5]: ");
    SetColor(7);
}

int main()
{
    // 设置控制台为UTF-8编码，解决中文乱码
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    // 初始化随机数种子（全局只需一次）
    srand((unsigned)time(NULL));

    system("mode con cols=110 lines=30");

    while (1)
    {
        showMainMenu();

        int choice = 0;
        scanf("%d", &choice);
        // 清除输入缓冲区
        int c; while ((c = getchar()) != '\n' && c != EOF) {}

        switch (choice)
        {
        case 1: // 注册
            if (!registerUser())
                printf("\n                                        注册失败，请重试。\n");
            printf("                                        按任意键继续...");
            system("pause>nul");
            break;

        case 2: // 登录
            loginUser();
            printf("                                        按任意键继续...");
            system("pause>nul");
            break;

        case 3: // 进入游戏
            if (currentUser[0] == '\0')
            {
                printf("\n                                        请先登录再进入游戏！\n");
                printf("                                        按任意键继续...");
                system("pause>nul");
                break;
            }
            do {
                afterGameChoice = 0;
                gamestart();
                gamecircle();
            } while (afterGameChoice == 1);
            break;

        case 4: // 查看日志
            showGameLog();
            break;

        case 5: // 退出
            system("cls");
            printf("\n  感谢使用，再见！\n\n");
            return 0;

        default:
            printf("  无效选项，请输入 1-5。\n");
            Sleep(800);
            break;
        }
    }
    return 0;
}
