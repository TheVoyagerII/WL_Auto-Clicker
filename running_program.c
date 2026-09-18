#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <string.h> //For memset
#include <signal.h>
#include <sys/select.h>
#include <unistd.h> // For usleep
#include <errno.h>
#include <limits.h>
#include <time.h>

/*
LMB - Left Mouse Button
RMB - Right Mouse Button
*/
long long NS_CAP = 2000000;
typedef struct hotkey_mappings
{
    char key_name[6];
    int key_idx;
} HM;

typedef struct button_and_cps
{
    int button;
    double cps;
} btn_n_cps;

void strip_trailing_newline(char *buf, size_t len)
{
    char *nl = memchr(buf, '\n', len);
    if (nl)
        *nl = '\0';
}

const HM mappings[] = {
    {"a", KEY_A},
    {"b", KEY_B},
    {"c", KEY_C},
    {"d", KEY_D},
    {"e", KEY_E},
    {"f", KEY_F},
    {"g", KEY_G},
    {"h", KEY_H},
    {"i", KEY_I},
    {"j", KEY_J},
    {"k", KEY_K},
    {"l", KEY_L},
    {"m", KEY_M},
    {"n", KEY_N},
    {"o", KEY_O},
    {"p", KEY_P},
    {"q", KEY_Q},
    {"r", KEY_R},
    {"s", KEY_S},
    {"t", KEY_T},
    {"u", KEY_U},
    {"v", KEY_V},
    {"w", KEY_W},
    {"x", KEY_X},
    {"y", KEY_Y},
    {"z", KEY_Z},
    {"ctrl", KEY_LEFTCTRL},
    {"shift", KEY_LEFTSHIFT},
    {"alt", KEY_LEFTALT},
    {"f1", KEY_F1},
    {"f2", KEY_F2},
    {"f3", KEY_F3},
    {"f4", KEY_F4},
    {"f5", KEY_F5},
    {"f6", KEY_F6},
    {"f7", KEY_F7},
    {"f8", KEY_F8},
    {"f9", KEY_F9},
    {"f10", KEY_F10},
    {"f11", KEY_F11},
    {"f12", KEY_F12},
    {"-", KEY_MINUS},
    {"=", KEY_EQUAL},
    {"[", KEY_LEFTBRACE},
    {"]", KEY_RIGHTBRACE},
    {"\\", KEY_BACKSLASH},
    {";", KEY_SEMICOLON},
    {"'", KEY_APOSTROPHE},
    {",", KEY_COMMA},
    {".", KEY_DOT},
    {"/", KEY_SLASH},
    {"`", KEY_GRAVE},
    {"0", KEY_0},
    {"1", KEY_1},
    {"2", KEY_2},
    {"3", KEY_3},
    {"4", KEY_4},
    {"5", KEY_5},
    {"6", KEY_6},
    {"7", KEY_7},
    {"8", KEY_8},
    {"9", KEY_9}};

int HOTKEY[3] = {-1, -1, -1};

sig_atomic_t autoclick_run = 0;
sig_atomic_t global_run = 1;
pthread_t main_pid;

static inline long long convert_to_ns(struct timespec *ts)
{
    return (long long)ts->tv_sec * 1000000000LL + ts->tv_nsec;
}

int hotkey_validator(const char hotkey_string[])
{
    int ret_val = 0;
    int i = 0, hk_idx = 0, mappings_count = sizeof(mappings) / sizeof(mappings[0]);
    bool hk_idx_lt3 = true;
    while (i < 15 && hotkey_string[i] != '\0' && hk_idx_lt3)
    {
        int j = i;
        while (j < 15 && hotkey_string[j] != '\0')
        {
            if (hotkey_string[j] == '+')
                break;
            j++;
        }
        char *present_substr = malloc((j - i) + 1);
        bool substr_found = false;
        strlcpy(present_substr, hotkey_string + i, j - i + 1);
        for (int map_i = 0; map_i < mappings_count; map_i++)
        {
            if (hk_idx > 2)
            {
                fprintf(stderr, "%s", "More than 3 keys in combination, truncating to first three...\n");
                hk_idx_lt3 = false;
                ret_val = 2;
                substr_found = true;
                break;
            }
            if (strcmp(present_substr, mappings[map_i].key_name) == 0)
            {
                HOTKEY[hk_idx++] = mappings[map_i].key_idx;
                substr_found = true;
                break;
            }
        }
        if (substr_found == false)
        {
            fprintf(stderr, "Error: Unidentified expression - %s\n", present_substr);
            return 1;
        }
        free(present_substr);
        if (j >= 15 || hotkey_string[j] == '\0')
            break;
        i = j + 1;
    }
    return ret_val;
}

int hotkey_populator(void)
{
    const char *hme = getenv("HOME");

    if (hme == NULL)
    {
        fprintf(stderr, "%s", "Error: Environment variable not set - HOME\n");
        return 1;
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/.config/WL_Auto-Clicker/HOTKEY.txt", hme);

    int hk_file = open(path, O_RDONLY);
    if (hk_file == -1)
    {
        fprintf(stderr, "%s", "Error: Couldn't open HOTKEY.txt\n");
        return 1;
    }

    char hotkey_string[15] = {0}; // Because largest hotkey, ctrl+shift+F1x, is 14 characters
    read(hk_file, &hotkey_string, sizeof(hotkey_string));

    strip_trailing_newline(hotkey_string, sizeof(hotkey_string));

    // Converting entire string to only lowercase characters to easy lookup later
    for (int i = 0; i < 15 && hotkey_string[i] != '\0'; i++)
        if (hotkey_string[i] >= 'A' && hotkey_string[i] <= 'Z')
            hotkey_string[i] = (hotkey_string[i] - 'A') + 'a';

    if (hotkey_validator(hotkey_string))
        return 1;

    close(hk_file);
    return 0;
}

void emit(int fd, int type, int code, int val)
{
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = val;

    write(fd, &ie, sizeof(ie));
}

void *clicker_func(void *arg)
{
    // Open uinput file to write to
    int clik_out_file = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (clik_out_file == -1)
    {
        perror("Couldn't open input file");
        fprintf(stderr, "%s", "Couldn't open uinput file, please make sure you have run the setup file and check you permission with ls -la /dev/uinput\n");
        global_run = 0;
        pthread_kill(main_pid, SIGINT);
        return NULL;
    }

    // Enabling left and right mouse button events
    ioctl(clik_out_file, UI_SET_EVBIT, EV_KEY);

    ioctl(clik_out_file, UI_SET_KEYBIT, BTN_LEFT);  // For LMB
    ioctl(clik_out_file, UI_SET_KEYBIT, BTN_RIGHT); // For RMB

    // Enabling axial movement to classify device as pointer
    ioctl(clik_out_file, UI_SET_EVBIT, EV_REL);
    ioctl(clik_out_file, UI_SET_RELBIT, REL_X);
    ioctl(clik_out_file, UI_SET_RELBIT, REL_Y);

    // Making click-write struct and cleaning everything to 0
    struct uinput_setup uin;
    memset(&uin, 0, sizeof(uin));

    // Populating input struct
    uin.id.bustype = BUS_VIRTUAL;
    uin.id.vendor = 0x0000;
    uin.id.product = 0x0000;
    strcpy(uin.name, "Auto-Clicking Mouse");

    // Creating the mouse
    ioctl(clik_out_file, UI_DEV_SETUP, &uin);
    ioctl(clik_out_file, UI_DEV_CREATE);

    btn_n_cps *param = (btn_n_cps *)arg;
    int button = param->button;
    double CPS = param->cps;
    long long ns_btw_clicks = (long long)(1e9 / CPS);
    // printf("%lld\n", us_btw_clicks);

    long long start_at_ns = 0ll;
    unsigned long long num_of_clks = 0;
    bool was_active = false;

    while (global_run)
    {
        if (autoclick_run)
        {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            long long now_in_ns = convert_to_ns(&ts);
            if (!was_active)
            {
                start_at_ns = now_in_ns;
                num_of_clks = 0ll;
                was_active = true;
            }
            long long next_clk_ns = start_at_ns + num_of_clks * (ns_btw_clicks);
            if (now_in_ns >= next_clk_ns)
            {
                if (button == 0)
                {
                    emit(clik_out_file, EV_KEY, BTN_LEFT, 1);
                    emit(clik_out_file, EV_SYN, SYN_REPORT, 0);
                    emit(clik_out_file, EV_KEY, BTN_LEFT, 0);
                    emit(clik_out_file, EV_SYN, SYN_REPORT, 0);
                }
                else
                {
                    emit(clik_out_file, EV_KEY, BTN_RIGHT, 1);
                    emit(clik_out_file, EV_SYN, SYN_REPORT, 0);
                    emit(clik_out_file, EV_KEY, BTN_RIGHT, 0);
                    emit(clik_out_file, EV_SYN, SYN_REPORT, 0);
                }
                num_of_clks++;
            }
            else
            {
                long long remain_ns = next_clk_ns - now_in_ns;
                long long sleep_for_ns = (remain_ns < NS_CAP) ? remain_ns : NS_CAP;
                struct timespec sleep_ts;
                sleep_ts.tv_nsec = sleep_for_ns % 1000000000LL;
                sleep_ts.tv_sec = sleep_for_ns / 1000000000LL;
                nanosleep(&sleep_ts, NULL);
            }
        }
        else
        {
            was_active = false;
            usleep(5000);
        }
    }
    ioctl(clik_out_file, UI_DEV_DESTROY);
    close(clik_out_file);
    return NULL;
}

void *keyboard_input_func(void *arg)
{
    bool keys[KEY_CNT] = {0};
    char eve_path[32];
    snprintf(eve_path, sizeof(eve_path), "/dev/input/event%s", (char *)arg);

    int key_in_file = open(eve_path, O_RDONLY);
    if (key_in_file == -1)
    {
        perror("Couldn't open input file");

        fprintf(stderr, "Couldn't open input file - %s, please check if event number is correct at ~/config/WL_Auto-Clicker/EVENT_NUM.txt\n", eve_path);
        global_run = 0;
        pthread_kill(main_pid, SIGINT);
        return NULL;
    }
    struct input_event ie;
    bool was_running = false;

    while (global_run)
    {
        read(key_in_file, &ie, sizeof(ie));
        keys[ie.code] = (ie.value == 0) ? false : true; // 1 for active, 0 for released
        switch (ie.code)
        {
        // case KEY_LEFTCTRL:
        //     keys[KEY_RIGHTCTRL] = keys[ie.code];
        //     break;
        case KEY_RIGHTCTRL:
            keys[KEY_LEFTCTRL] = keys[ie.code];
            break;
        // case KEY_LEFTSHIFT:
        //     keys[KEY_RIGHTSHIFT] = keys[ie.code];
        //     break;
        case KEY_RIGHTSHIFT:
            keys[KEY_LEFTSHIFT] = keys[ie.code];
            break;
        // case KEY_LEFTALT:
        //     keys[KEY_RIGHTALT] = keys[ie.code];
        //     break;
        case KEY_RIGHTALT:
            keys[KEY_LEFTALT] = keys[ie.code];
            break;
        default:
            break;
        }
        bool now_running = true;
        for (int i = 0; i < 3; i++) // When all keys pressed down, toggle state
            if (HOTKEY[i] != -1 && keys[HOTKEY[i]] == false)
                now_running = false;

        if (now_running && !was_running)
        {
            autoclick_run ^= 1; // Toggle the Auto-clicker
            if (autoclick_run)
                fprintf(stdout, "Auto Clicker Running...\n");
            else
                fprintf(stdout, "Auto Clicker Stopped.\n");
            was_running = true;
        }
        else if (!now_running && was_running)
        {
            was_running = false;
        }
    }
    close(key_in_file);
    return NULL;
}

void sig_handler(int sig)
{
    if (sig == SIGINT || sig == SIGHUP)
        global_run = 0;
}

int main(int argc, char *argv[])
{
    main_pid = pthread_self();
    const char help[] = "Usage : ./WL_Auto-Clicker --hotkey <combination-string>\n./WL_Auto-Clicker --start <button> <cps>\n";
    // ----------------------Flag Handling-------------------------

    // Too few arguments or --help called inherently
    if (argc <= 2)
    {
        fprintf(stdout, "%s", help);
        return 0;
    }
    // Incorrect Flags
    if (strcmp(argv[1], "--start") != 0 && strcmp(argv[1], "--hotkey") != 0) // Flag type check
    {
        fprintf(stderr, "%s", help);
        return 0;
    }

    const char *hme = getenv("HOME");

    if (hme == NULL)
    {
        fprintf(stderr, "%s", "Error: Environment variable not set - HOME\n");
        return 0;
    }

    // ---------hotkey flag control------------
    if (strcmp(argv[1], "--hotkey") == 0)
    {
        // Empty string for hotkey
        if (strcmp(argv[2], "") == 0)
        {
            fprintf(stderr, "%s", "Error: Please enter valid hotkey combination. Example of valid combination: \"ctrl+shift+r\"\n");
            return 0;
        }
        int c_count = 0;
        while (argv[2][c_count] != '\0')
            c_count++;
        // hotkey too long (Incorrect combination)
        if (c_count > 14)
        {
            fprintf(stderr, "%s", "Error: Input combination too large. Please enter valid hotkey combination. Example of valid combination: \"ctrl+shift+r\"\n");
            return 0;
        }

        const char *hme = getenv("HOME");
        if (hme == NULL)
        {
            fprintf(stderr, "%s", "Error: Environment variable not set - HOME\n");
            return 0;
        }
        char path[1024];
        snprintf(path, sizeof(path), "%s/.config/WL_Auto-Clicker/HOTKEY.txt", hme);
        int hk_file = open(path, O_WRONLY | O_TRUNC);
        if (hk_file == -1)
        {
            fprintf(stderr, "%s", "Error: Couldn't open HOTKEY.txt\n");
            return 0;
        }
        write(hk_file, argv[2], strlen(argv[2]));

        printf("Entered hotkey : %s.\n", argv[2]);
        close(hk_file);

        int hv = hotkey_validator(argv[2]);
        if (hv == 1)
            return 1;
        else if (hv == 2)
        {
            char *hk_hold_print = malloc(sizeof(char) * 15);
            int plus_cnt = 0;
            for (int i = 0; i < sizeof(argv[2]) / sizeof(argv[2][0]); i++)
            {
                if (argv[2][i] == '+')
                    plus_cnt++;
                if (plus_cnt == 3)
                    break;
                hk_hold_print[i] = argv[2][i];
            }
            fprintf(stdout, "Successfully changed hotkey to %s.\n", hk_hold_print);
            free(hk_hold_print);
        }
        else
            fprintf(stdout, "Successfully changed hotkey to %s.\n", argv[2]);
        return 0;
    }
    // -----------start flag control
    else if (strcmp(argv[1], "--start") == 0)
    {
        btn_n_cps bnc; // Button and CPS
        if (strcmp(argv[2], "lmb") != 0 && strcmp(argv[2], "rmb") != 0)
        {
            fprintf(stderr, "Error: Unknown button %s. Please use \"lmb\" or \"rmb\".\n", argv[2]); // Button Check
            return 0;
        }
        else if (strcmp(argv[2], "lmb") == 0)
            bnc.button = 0;
        else
            bnc.button = 1;

        errno = 0; // All parameters to verify CPS
        char *endptr;
        double cl_ps = strtold(argv[3], &endptr);
        if (errno == ERANGE)
        {
            fprintf(stderr, "%s", "Error: Please choose cps between 0.01 and 1000000\n");
            return 0;
        }
        if (endptr == argv[3] || *endptr != '\0') // No conversion, no digit found
        {
            fprintf(stderr, "%s", "Error: Please choose cps as a number between 0.01 and 1000000\n");
            return 0;
        }
        if (cl_ps < 0.01 || cl_ps > 1000000)
        {
            fprintf(stderr, "%s", "Error: Please choose cps between 0.01 and 1000000\n");
            return 0;
        }
        if (cl_ps > 2000)
        { // Safety check
            printf("%s\n", "WARNING: Choosing CPS higher than 2000 has been shown in tests to severly slow down or even crash programs. Proceed at your own discertion.\nContinue? [y/N]: ");
            char high_cps_option = 'n';
            scanf("%c", &high_cps_option);
            if (high_cps_option == 'y' || high_cps_option == 'Y')
            {
                printf("Proceeding with high CPS...\n");
            }
            else
            {
                printf("Safely exited a dangerous start. Please try a lower CPS value.'\n");
                return 0;
            }
        }
        bnc.cps = cl_ps;
        if (hotkey_populator())
        {
            fprintf(stderr, "%s", "Error in hotkey setup, exiting main...\n");
            return 0;
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/.config/WL_Auto-Clicker/EVENT_NUM.txt", hme);

        int eve_file = open(path, O_RDONLY);
        if (eve_file == -1)
        {
            fprintf(stderr, "%s", "Error: Couldn't open EVENT_NUM.txt\n");
            return 0;
        }
        printf("Press hotkey to start the auto-clicker...\n");
        char eve_num[8] = {0};
        read(eve_file, eve_num, sizeof(eve_num) - 1);
        strip_trailing_newline(eve_num, sizeof(eve_num));
        close(eve_file);

        signal(SIGINT, &sig_handler);
        signal(SIGHUP, &sig_handler);

        pthread_t keyboard_input;
        pthread_create(&keyboard_input, NULL, keyboard_input_func, eve_num);
        pthread_t clicker;
        pthread_create(&clicker, NULL, clicker_func, &bnc);

        while (global_run)
            pause();

        pthread_join(keyboard_input, NULL);
        pthread_join(clicker, NULL);
    }
    return 0;
}