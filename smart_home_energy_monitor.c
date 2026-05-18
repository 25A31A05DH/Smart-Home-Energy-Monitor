/*
 * ============================================================
 *        SMART HOME ENERGY MONITOR
 *        Programming-based Energy Monitoring System
 * ============================================================
 * Tracks household appliance electricity consumption,
 * calculates total energy (kWh) and cost (INR),
 * and suggests which appliances use the most power.
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Constants ─────────────────────────────────────────── */
#define MAX_APPLIANCES   50
#define MAX_NAME_LEN     64
#define TARIFF_PER_KWH   12.0   /* ₹12 per kWh (Indian standard) */
#define HIGH_USAGE_KWH   10.0   /* Flag appliances above this threshold */
#define INPUT_FILE       "appliances.txt"
#define REPORT_FILE      "energy_report.txt"

/* ── Data structure for one appliance ──────────────────── */
typedef struct {
    char   name[MAX_NAME_LEN]; /* Appliance name                   */
    double watts;              /* Power rating in Watts            */
    double hours;              /* Daily usage in hours             */
    double kwh;                /* Calculated kWh = (W × h) / 1000 */
    double cost;               /* Cost in ₹                        */
} Appliance;

/* ── Function prototypes ────────────────────────────────── */
void  print_banner(void);
int   load_appliances(const char *filename, Appliance arr[], int *count);
void  calculate_energy(Appliance arr[], int count);
void  sort_by_kwh(Appliance arr[], int count);
void  print_summary(const Appliance arr[], int count);
void  print_recommendations(const Appliance arr[], int count);
void  save_report(const Appliance arr[], int count);
void  create_sample_file(const char *filename);
void  print_separator(char ch, int width);

/* ══════════════════════════════════════════════════════════
 *  MAIN
 * ══════════════════════════════════════════════════════════ */
int main(void)
{
    Appliance appliances[MAX_APPLIANCES];
    int count = 0;

    print_banner();

    /* ── Try to open the input file; create sample if missing ── */
    FILE *test = fopen(INPUT_FILE, "r");
    if (!test) {
        printf("\n  [INFO] '%s' not found — creating a sample file...\n", INPUT_FILE);
        create_sample_file(INPUT_FILE);
        printf("  [INFO] Sample file created. You can edit it and re-run the program.\n\n");
    } else {
        fclose(test);
    }

    /* ── Load data ─────────────────────────────────────────── */
    if (!load_appliances(INPUT_FILE, appliances, &count)) {
        fprintf(stderr, "\n  [ERROR] Failed to load data from '%s'. Exiting.\n", INPUT_FILE);
        return EXIT_FAILURE;
    }

    /* ── Process ────────────────────────────────────────────── */
    calculate_energy(appliances, count);
    sort_by_kwh(appliances, count);

    /* ── Display ────────────────────────────────────────────── */
    print_summary(appliances, count);
    print_recommendations(appliances, count);

    /* ── Save report ─────────────────────────────────────────── */
    save_report(appliances, count);

    printf("\n  [INFO] Detailed report saved to '%s'\n", REPORT_FILE);
    print_separator('=', 60);
    printf("\n");

    return EXIT_SUCCESS;
}

/* ══════════════════════════════════════════════════════════
 *  BANNER
 * ══════════════════════════════════════════════════════════ */
void print_banner(void)
{
    print_separator('=', 60);
    printf("   SMART HOME ENERGY MONITOR\n");
    printf("   Programming-based Energy Monitoring System\n");
    print_separator('=', 60);
    printf("   Tariff Rate : Rs. %.2f per kWh\n", TARIFF_PER_KWH);
    printf("   Input File  : %s\n", INPUT_FILE);
    print_separator('-', 60);
}

/* ══════════════════════════════════════════════════════════
 *  FILE LOADING
 *  Expected file format (one appliance per line):
 *  ApplianceName  Watts  HoursPerDay
 *  Lines starting with '#' are comments and are skipped.
 * ══════════════════════════════════════════════════════════ */
int load_appliances(const char *filename, Appliance arr[], int *count)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("  fopen");
        return 0;
    }

    *count = 0;
    char line[256];
    int  line_num = 0;

    printf("\n  Loading appliance data from '%s'...\n\n", filename);
    printf("  %-30s %10s %12s\n", "Appliance", "Watts", "Hours/Day");
    print_separator('-', 60);

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        /* Skip blank lines and comments */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;

        /* Remove trailing newline */
        line[strcspn(line, "\r\n")] = '\0';

        Appliance *a = &arr[*count];
        if (sscanf(line, "%63s %lf %lf", a->name, &a->watts, &a->hours) != 3) {
            fprintf(stderr, "  [WARN] Line %d skipped (bad format): %s\n", line_num, line);
            continue;
        }

        if (a->watts <= 0 || a->hours < 0 || a->hours > 24) {
            fprintf(stderr, "  [WARN] Line %d skipped (invalid values): %s\n", line_num, line);
            continue;
        }

        printf("  %-30s %10.1f %12.1f\n", a->name, a->watts, a->hours);
        (*count)++;

        if (*count >= MAX_APPLIANCES) {
            printf("  [WARN] Maximum appliance limit (%d) reached.\n", MAX_APPLIANCES);
            break;
        }
    }

    fclose(fp);
    print_separator('-', 60);
    printf("  %d appliance(s) loaded successfully.\n", *count);

    return (*count > 0);
}

/* ══════════════════════════════════════════════════════════
 *  ENERGY CALCULATION
 *  Formula: kWh = (Watts × Hours) / 1000
 *  Cost    = kWh × TARIFF_PER_KWH
 * ══════════════════════════════════════════════════════════ */
void calculate_energy(Appliance arr[], int count)
{
    for (int i = 0; i < count; i++) {
        arr[i].kwh  = (arr[i].watts * arr[i].hours) / 1000.0;
        arr[i].cost = arr[i].kwh * TARIFF_PER_KWH;
    }
}

/* ══════════════════════════════════════════════════════════
 *  SORTING — Bubble sort (descending kWh)
 * ══════════════════════════════════════════════════════════ */
void sort_by_kwh(Appliance arr[], int count)
{
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - 1 - i; j++) {
            if (arr[j].kwh < arr[j + 1].kwh) {
                Appliance temp = arr[j];
                arr[j]         = arr[j + 1];
                arr[j + 1]     = temp;
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════
 *  SUMMARY TABLE
 * ══════════════════════════════════════════════════════════ */
void print_summary(const Appliance arr[], int count)
{
    double total_kwh  = 0.0;
    double total_cost = 0.0;

    printf("\n");
    print_separator('=', 60);
    printf("   DAILY ENERGY CONSUMPTION REPORT\n");
    print_separator('=', 60);

    printf("  %-4s %-26s %8s %10s %10s\n",
           "Rank", "Appliance", "kWh", "Cost(Rs)", "Flag");
    print_separator('-', 60);

    for (int i = 0; i < count; i++) {
        const char *flag = (arr[i].kwh >= HIGH_USAGE_KWH) ? "*** HIGH" : "";
        printf("  %-4d %-26s %8.3f %10.2f %10s\n",
               i + 1,
               arr[i].name,
               arr[i].kwh,
               arr[i].cost,
               flag);
        total_kwh  += arr[i].kwh;
        total_cost += arr[i].cost;
    }

    print_separator('-', 60);
    printf("  %-4s %-26s %8.3f %10.2f\n",
           "", "TOTAL (per day)", total_kwh, total_cost);

    double monthly_kwh  = total_kwh  * 30.0;
    double monthly_cost = total_cost * 30.0;

    print_separator('-', 60);
    printf("  %-4s %-26s %8.3f %10.2f\n",
           "", "TOTAL (per month ~30d)", monthly_kwh, monthly_cost);
    print_separator('=', 60);

    /* ── Mini dashboard ──────────────────────────────────── */
    printf("\n  DASHBOARD SUMMARY\n");
    print_separator('-', 40);
    printf("  Daily  kWh Consumed   : %.3f kWh\n",  total_kwh);
    printf("  Daily  Estimated Cost : Rs. %.2f\n",   total_cost);
    printf("  Monthly kWh Consumed  : %.2f kWh\n",  monthly_kwh);
    printf("  Monthly Estimated Cost: Rs. %.2f\n",   monthly_cost);
    printf("  Tariff Rate Applied   : Rs. %.2f/kWh\n", TARIFF_PER_KWH);

    /* Highest consumer */
    if (count > 0)
        printf("  Highest Consumer      : %s (%.3f kWh/day)\n",
               arr[0].name, arr[0].kwh);
}

/* ══════════════════════════════════════════════════════════
 *  RECOMMENDATIONS
 * ══════════════════════════════════════════════════════════ */
void print_recommendations(const Appliance arr[], int count)
{
    printf("\n");
    print_separator('=', 60);
    printf("   ENERGY OPTIMIZATION RECOMMENDATIONS\n");
    print_separator('=', 60);

    int high_count = 0;

    for (int i = 0; i < count; i++) {
        if (arr[i].kwh < HIGH_USAGE_KWH) break; /* sorted, so we can stop */
        high_count++;

        printf("\n  [%d] %s  (%.3f kWh/day | Rs. %.2f/day)\n",
               i + 1, arr[i].name, arr[i].kwh, arr[i].cost);

        /* Generic advice based on common appliance keywords */
        char lower[MAX_NAME_LEN];
        strncpy(lower, arr[i].name, MAX_NAME_LEN - 1);
        lower[MAX_NAME_LEN - 1] = '\0';
        for (int k = 0; lower[k]; k++)
            if (lower[k] >= 'A' && lower[k] <= 'Z') lower[k] += 32;

        if (strstr(lower, "ac") || strstr(lower, "air")) {
            printf("      -> Set thermostat to 24-26C instead of 18-20C.\n");
            printf("      -> Use fan mode when possible; avoid leaving AC on standby.\n");
        } else if (strstr(lower, "heater") || strstr(lower, "geyser")) {
            printf("      -> Reduce heating time; use a timer to auto-shutoff.\n");
            printf("      -> Insulate the water tank to retain heat longer.\n");
        } else if (strstr(lower, "fridge") || strstr(lower, "refrigerator")) {
            printf("      -> Keep coils clean and ensure door seals are tight.\n");
            printf("      -> Set temperature to recommended 3-5C (fridge) / -18C (freezer).\n");
        } else if (strstr(lower, "wash")) {
            printf("      -> Use cold-water cycles; run only with full loads.\n");
            printf("      -> Air-dry clothes instead of using the dryer.\n");
        } else if (strstr(lower, "tv") || strstr(lower, "television")) {
            printf("      -> Reduce screen brightness and use sleep timer.\n");
            printf("      -> Unplug when not in use to avoid standby consumption.\n");
        } else if (strstr(lower, "light") || strstr(lower, "lamp")) {
            printf("      -> Replace with LED bulbs (80%% less energy).\n");
            printf("      -> Use motion sensors and natural daylight when possible.\n");
        } else {
            printf("      -> Reduce daily usage by 1-2 hours to save ~Rs. %.2f/month.\n",
                   arr[i].cost * 0.15 * 30);
            printf("      -> Switch off completely when not in use (avoid standby).\n");
        }
    }

    if (high_count == 0) {
        printf("\n  All appliances are within acceptable consumption limits.\n");
        printf("  No major optimization needed — great energy discipline!\n");
    } else {
        double potential_daily = 0.0;
        for (int i = 0; i < count && arr[i].kwh >= HIGH_USAGE_KWH; i++)
            potential_daily += arr[i].cost * 0.15; /* 15% reduction estimate */

        printf("\n  [ESTIMATE] Reducing high-usage appliances by ~15%%:\n");
        printf("      -> Saves ~Rs. %.2f/day\n",          potential_daily);
        printf("      -> Saves ~Rs. %.2f/month\n",        potential_daily * 30);
        printf("      -> Saves ~Rs. %.2f/year\n",         potential_daily * 365);
    }

    /* General tips */
    printf("\n  GENERAL TIPS\n");
    print_separator('-', 40);
    printf("  * Unplug chargers and electronics when not in use.\n");
    printf("  * Use BEE 5-star rated appliances for efficiency.\n");
    printf("  * Schedule heavy appliances during off-peak hours.\n");
    printf("  * Install smart plugs for real-time monitoring.\n");
}

/* ══════════════════════════════════════════════════════════
 *  SAVE TEXT REPORT
 * ══════════════════════════════════════════════════════════ */
void save_report(const Appliance arr[], int count)
{
    FILE *fp = fopen(REPORT_FILE, "w");
    if (!fp) {
        fprintf(stderr, "  [WARN] Could not write report to '%s'.\n", REPORT_FILE);
        return;
    }

    double total_kwh  = 0.0;
    double total_cost = 0.0;
    for (int i = 0; i < count; i++) {
        total_kwh  += arr[i].kwh;
        total_cost += arr[i].cost;
    }

    fprintf(fp, "SMART HOME ENERGY MONITOR - REPORT\n");
    fprintf(fp, "====================================\n\n");
    fprintf(fp, "Tariff: Rs. %.2f per kWh\n\n", TARIFF_PER_KWH);
    fprintf(fp, "%-4s %-26s %8s %10s %12s %10s\n",
            "Rank", "Appliance", "Watts", "Hours/Day", "kWh/Day", "Cost(Rs)");
    fprintf(fp, "-----------------------------------------------------------------------\n");

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%-4d %-26s %8.1f %10.1f %12.3f %10.2f\n",
                i + 1, arr[i].name, arr[i].watts, arr[i].hours,
                arr[i].kwh, arr[i].cost);
    }

    fprintf(fp, "-----------------------------------------------------------------------\n");
    fprintf(fp, "%-4s %-26s %8s %10s %12.3f %10.2f\n",
            "", "DAILY TOTAL", "", "", total_kwh, total_cost);
    fprintf(fp, "%-4s %-26s %8s %10s %12.3f %10.2f\n",
            "", "MONTHLY TOTAL (~30 days)", "", "",
            total_kwh * 30, total_cost * 30);
    fprintf(fp, "\n-- Report generated by Smart Home Energy Monitor (C) --\n");

    fclose(fp);
}

/* ══════════════════════════════════════════════════════════
 *  CREATE SAMPLE INPUT FILE
 *  Format: ApplianceName  Watts  HoursPerDay
 * ══════════════════════════════════════════════════════════ */
void create_sample_file(const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("  fopen (create sample)");
        return;
    }

    fprintf(fp, "# Smart Home Energy Monitor - Appliance Data File\n");
    fprintf(fp, "# Format: ApplianceName  Watts  HoursPerDay\n");
    fprintf(fp, "# Lines beginning with '#' are comments.\n");
    fprintf(fp, "#\n");
    fprintf(fp, "# Appliance               Watts   Hours/Day\n");
    fprintf(fp, "Air_Conditioner           1500    8\n");
    fprintf(fp, "Water_Heater_Geyser       2000    1\n");
    fprintf(fp, "Refrigerator              150     24\n");
    fprintf(fp, "Washing_Machine           500     1\n");
    fprintf(fp, "Television                120     5\n");
    fprintf(fp, "Ceiling_Fan               75      10\n");
    fprintf(fp, "LED_Lights                60      6\n");
    fprintf(fp, "Laptop                    65      8\n");
    fprintf(fp, "Microwave_Oven            1200    0.5\n");
    fprintf(fp, "Electric_Iron             1000    0.5\n");
    fprintf(fp, "WiFi_Router               10      24\n");
    fprintf(fp, "Phone_Charger             10      2\n");

    fclose(fp);
}

/* ══════════════════════════════════════════════════════════
 *  UTILITY — print separator line
 * ══════════════════════════════════════════════════════════ */
void print_separator(char ch, int width)
{
    for (int i = 0; i < width; i++) putchar(ch);
    putchar('\n');
}
