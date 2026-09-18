#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <string.h> // For memset
#include <unistd.h>

typedef struct benchtest_struct
{
    double target_cps;
    double achieved_cps;
    double deviation_pct;
    unsigned long long num_clicks_fired;
    double duration_sec;
} bt_str;

static inline long long convert_to_ns(struct timespec *ts)
{
    return (long long)ts->tv_sec * 1000000000LL + ts->tv_nsec;
}

void emit(int fd, int type, int code, int val)
{
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = val;

    write(fd, &ie, sizeof(ie));
}

long long NS_CAP = 2000000LL;

bt_str run_benchtest(double target_cps, double duration_sec, bool emit_real_events)
{
    long long interval_ns = (long long)(1e9 / target_cps);

    int clik_file = -1;
    if (emit_real_events)
    {
        int clik_file = open("/dev/uinput", O_RDONLY | O_NONBLOCK);
        if (clik_file == -1)
        {
            perror("Unable to open input file. Performing only sched-type benchmarking...\n");
            emit_real_events = false;
        }
        else
        {
            ioctl(clik_file, UI_SET_EVBIT, EV_KEY);

            ioctl(clik_file, UI_SET_KEYBIT, BTN_LEFT); // For LMB

            // Enabling axial movement to classify device as pointer
            ioctl(clik_file, UI_SET_EVBIT, EV_REL);
            ioctl(clik_file, UI_SET_RELBIT, REL_X);
            ioctl(clik_file, UI_SET_RELBIT, REL_Y);

            // Making click-write struct and cleaning everything to 0
            struct uinput_setup uin;
            memset(&uin, 0, sizeof(uin));

            // Populating input struct
            uin.id.bustype = BUS_VIRTUAL;
            uin.id.vendor = 0x0000;
            uin.id.product = 0x0000;
            strcpy(uin.name, "Auto-Clicking Mouse TestBench");

            // Creating the mouse
            ioctl(clik_file, UI_DEV_SETUP, &uin);
            ioctl(clik_file, UI_DEV_CREATE);
        }
    }

    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    long long start_ns = convert_to_ns(&start_ts);
    long long end_ns = start_ns + (1000000000LL) * duration_sec;

    unsigned long long num_clks = 0;
    while (1)
    {
        struct timespec now_ts;
        clock_gettime(CLOCK_MONOTONIC, &now_ts);
        long long now_ns = convert_to_ns(&now_ts);

        if (now_ns >= end_ns)
            break;

        long long next_ns = start_ns + num_clks * (interval_ns);
        if (now_ns >= next_ns)
        {
            if (clik_file != -1)
            {
                emit(clik_file, EV_KEY, BTN_LEFT, 1);
                emit(clik_file, EV_SYN, SYN_REPORT, 0);
                emit(clik_file, EV_KEY, BTN_LEFT, 0);
                emit(clik_file, EV_SYN, SYN_REPORT, 0);
            }
            num_clks++;
        }
        else
        {
            long long remain_ns = next_ns - now_ns;
            long long sleep_for_ns = (remain_ns < NS_CAP) ? remain_ns : NS_CAP;
            struct timespec sleep_ts;
            sleep_ts.tv_nsec = sleep_for_ns % 1000000000LL;
            sleep_ts.tv_sec = sleep_for_ns / 1000000000LL;
            nanosleep(&sleep_ts, NULL);
        }
    }

    if (clik_file != -1)
    {
        ioctl(clik_file, UI_DEV_DESTROY);
        close(clik_file);
    }

    struct timespec actual_end_ts;
    clock_gettime(CLOCK_MONOTONIC, &actual_end_ts);
    long long duration_ns = (convert_to_ns(&actual_end_ts) - start_ns) / 1e9;

    bt_str output;
    output.target_cps = target_cps;
    output.duration_sec = duration_ns;
    output.num_clicks_fired = num_clks;
    output.achieved_cps = num_clks / duration_sec;
    output.deviation_pct = ((output.achieved_cps - target_cps) / target_cps) * 100.0;
    return output;
}

int main()
{
    int test_cps[] = {1, 2, 5, 10, 50, 100, 500, 1000, 5000, 10000, 50000, 100000, 1000000};
    int num_tests = sizeof(test_cps) / sizeof(test_cps[0]);

    printf("~~~~~~~~~~~~~~~~~~Only Scheduling~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("%-10s | %-12s | %-10s | %-10s | %14s\n", "Target CPS", "Achieved CPS", "Duration (in sec)", "Number of Clicks Fired", "Deviation %(Achieved to Target)");
    printf("---------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < num_tests; i++)
    {
        bt_str result = run_benchtest(test_cps[i], 5.0, false);
        printf("%-10.2f | %-12.2f | %-17.2f | %-22llu | %-+12.9f\n", result.target_cps, result.achieved_cps, result.duration_sec, result.num_clicks_fired, result.deviation_pct);
    }

    printf("~~~~~~~~~~~~~~~~~~Real Click Emits~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("%-10s | %-12s | %-10s | %-10s | %14s\n", "Target CPS", "Achieved CPS", "Duration (in sec)", "Number of Clicks Fired", "Deviation %(Achieved to Target)");
    printf("---------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < num_tests; i++)
    {
        bt_str result = run_benchtest(test_cps[i], 5.0, true);
        printf("%-10.2f | %-12.2f | %-17.2f | %-22llu | %-+12.9f\n", result.target_cps, result.achieved_cps, result.duration_sec, result.num_clicks_fired, result.deviation_pct);
    }
    return 0;
}