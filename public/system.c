#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_REPORTS 128
#define MAX_ZONES 10
#define PORT 8080
#define BUFFER_SIZE 8192

#define ANKLE_YELLOW_PCT  0.15f
#define KNEE_YELLOW_PCT   0.10f
#define KNEE_RED_PCT      0.25f
#define WAIST_YELLOW_PCT  0.05f
#define WAIST_RED_PCT     0.15f

struct reportstruct {
    int flood;
    int rain;
    int zone;
};

struct Zone {
    int Zonenumber;
    int houses;
};

struct Admin {
    char username[30];
    char password[30];
};

// Function prototypes
void reporting(struct reportstruct*, int*);
void loadZoneData(struct Zone zones[], int *zoneCount);
void loadReportData(struct reportstruct reports[], int *reportCount);
int get_int(const char *input_message);
void clear_stdin(void);
int read_report_file(struct reportstruct*, int*);
void PassageAnnouncement();
void pauseScreen();
void rewriteReportFile(struct reportstruct*, int);
void start_web_server(struct reportstruct *report_lists, int *number_of_lists);
void handle_client(SOCKET client_socket, struct reportstruct *report_lists, int *number_of_lists);
void send_file(SOCKET client, const char *filename, const char *content_type);
void send_json(SOCKET client, const char *json);
void send_error(SOCKET client, int code, const char *msg);
void url_decode(char *dst, const char *src);

void pauseScreen() {
    printf("\n\nPress Enter to continue...");
    while (getchar() != '\n');
}

void rewriteReportFile(struct reportstruct *report_lists, int number_of_lists) {
    FILE *fp = fopen("report.txt", "w");
    if (fp == NULL) {
        printf("\n❌ Error: Cannot rewrite report file!\n");
        return;
    }
    for (int i = 0; i < number_of_lists; i++) {
        fprintf(fp, "%d %d %d\n", report_lists[i].flood, report_lists[i].rain, report_lists[i].zone);
    }
    fclose(fp);
}

void loadZoneData(struct Zone zones[], int *zoneCount) {
    FILE *fp = fopen("zone.txt", "r");
    if (fp == NULL) {
        *zoneCount = 0;
        return;
    }
    *zoneCount = 0;
    while (fscanf(fp, "%d %d", &zones[*zoneCount].Zonenumber, &zones[*zoneCount].houses) == 2) {
        (*zoneCount)++;
    }
    fclose(fp);
}

void loadReportData(struct reportstruct reports[], int *reportCount) {
    FILE *fp = fopen("report.txt", "r");
    if (fp == NULL) {
        *reportCount = 0;
        return;
    }
    *reportCount = 0;
    while (*reportCount < MAX_REPORTS &&
           fscanf(fp, "%d %d %d", &reports[*reportCount].flood, &reports[*reportCount].rain, &reports[*reportCount].zone) == 3) {
        (*reportCount)++;
    }
    fclose(fp);
}

int read_report_file(struct reportstruct *report_lists, int *number_of_report_lists) {
    FILE *fp_report = fopen("report.txt", "r");
    if (fp_report == NULL) {
        fp_report = fopen("report.txt", "w");
        if (fp_report) fclose(fp_report);
        fp_report = fopen("report.txt", "r");
    }
    *number_of_report_lists = 0;
    while (*number_of_report_lists < MAX_REPORTS &&
           fscanf(fp_report, "%d %d %d",
                  &report_lists[*number_of_report_lists].flood,
                  &report_lists[*number_of_report_lists].rain,
                  &report_lists[*number_of_report_lists].zone) == 3) {
        (*number_of_report_lists)++;
    }
    fclose(fp_report);
    return 0;
}

void clear_stdin(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

int get_int(const char *input_message) {
    char input[20];
    char *ptrfirstnotnumber;
    long integer;
    while (1) {
        printf("%s", input_message);
        if (!fgets(input, sizeof(input), stdin)) continue;
        integer = strtol(input, &ptrfirstnotnumber, 10);
        if (strchr(input, '\n') == NULL) {
            clear_stdin();
            printf("Invalid Input. Input too long. Try again.\n");
            continue;
        }
        if (ptrfirstnotnumber == input || *ptrfirstnotnumber != '\n') {
            printf("Invalid Input. Input must not contain non-numerical characters. Try again.\n");
            continue;
        }
        return integer;
    }
}

void reporting(struct reportstruct *report_lists, int *number_of_lists) {
    int flood_lvl, rain_lvl, location;
    FILE *fp_report = fopen("report.txt", "a");
    if (fp_report == NULL) {
        printf("\nFile Error!");
        pauseScreen();
        return;
    }

    while (1) {
        int choice;
        printf("\n\n======= FLOOD REPORT =======\n");
        printf("\n\nOptions:\n\t[1] Report\n\t[2] View all Reports\n\t[3] Exit\n");
        while (1) {
            choice = get_int("Enter Choice: ");
            if (choice < 1 || choice > 3) {
                printf("Invalid Option. Try Again\n");
                continue;
            } else break;
        }

        while (choice == 1) {
            char another_report_decision;

            printf("\n\nFlood Level:\n\t[1] Ankle Level\n\t[2] Knee Level\n\t[3] Waist Level\n\n");
            while (1) {
                flood_lvl = get_int("Input Flood Level: ");
                if (flood_lvl > 3 || flood_lvl <= 0) printf("#%d is not an option. Try again", flood_lvl);
                else break;
            }

            printf("\n\nRain Intensity:\n\t[1] Light Rain\n\t[2] Moderate Rain\n\t[3] Heavy Rain\n\n");
            while (1) {
                rain_lvl = get_int("Input Rain Intensity: ");
                if (rain_lvl > 3 || rain_lvl <= 0) printf("#%d is not an option. Try again", rain_lvl);
                else break;
            }

            printf("\n\nLocated Zone:\n\tThis Location has 1-3 zones.\n\n");
            while (1) {
                location = get_int("Input Located Zone: ");
                if (location > 3 || location <= 0) printf("#%d is not an option. Try again", location);
                else break;
            }

            printf("\n\nReport Summary:\n\tFlood Level: %d\n\tRain Intensity: %d\n\tLocated Zone: %d\n",
                flood_lvl, rain_lvl, location);

            while (1) {
                char decision;
                printf("Enter 'S' to save report or enter 'D' to discard report: ");
                scanf(" %c", &decision);
                clear_stdin();

                if (decision != 'S' && decision != 'D') {
                    printf("Invalid Input, Try Again.\n");
                    continue;
                } else if (decision == 'S') {
                    if (*number_of_lists >= MAX_REPORTS) {
                        printf("Report list is full. Cannot save more reports.\n");
                        break;
                    }

                    struct Zone zones[MAX_ZONES];
                    int zoneCount = 0;
                    loadZoneData(zones, &zoneCount);

                    int zoneHouses = 0;
                    for (int z = 0; z < zoneCount; z++) {
                        if (zones[z].Zonenumber == location) {
                            zoneHouses = zones[z].houses;
                            break;
                        }
                    }

                    int zoneReportCount = 0;
                    for (int i = 0; i < *number_of_lists; i++) {
                        if (report_lists[i].zone == location) zoneReportCount++;
                    }

                    int zoneReportLimit = zoneHouses;
                    if (zoneReportLimit < 1) zoneReportLimit = 1;

                    if (zoneReportCount >= zoneReportLimit) {
                        printf("\n⚠️ Zone %d is FULL (%d/%d reports).\n", location, zoneReportCount, zoneReportLimit);
                        int oldestIndex = -1;
                        for (int i = 0; i < *number_of_lists; i++) {
                            if (report_lists[i].zone == location) {
                                oldestIndex = i;
                                break;
                            }
                        }
                        if (oldestIndex != -1) {
                            report_lists[oldestIndex].flood = flood_lvl;
                            report_lists[oldestIndex].rain = rain_lvl;
                            report_lists[oldestIndex].zone = location;
                            rewriteReportFile(report_lists, *number_of_lists);
                            printf("✅ Report overwritten! Zone %d: %d/%d\n", location, zoneReportCount, zoneReportLimit);
                        }
                        break;
                    }

                    printf("Saving Report...\n");
                    fprintf(fp_report, "%d %d %d\n", flood_lvl, rain_lvl, location);
                    fflush(fp_report);
                    report_lists[*number_of_lists].flood = flood_lvl;
                    report_lists[*number_of_lists].rain = rain_lvl;
                    report_lists[*number_of_lists].zone = location;
                    (*number_of_lists)++;
                    printf("✅ Report Saved! (%d/%d for Zone %d)\n", zoneReportCount + 1, zoneReportLimit, location);
                    break;
                } else if (decision == 'D') {
                    printf("Discarding Report...\n");
                    break;
                }
            }

            while (1) {
                char decision;
                printf("Enter 'Y' for another Report or enter 'N' to exit: ");
                scanf(" %c", &decision);
                clear_stdin();
                if (decision != 'Y' && decision != 'N') {
                    printf("Invalid Input, Try Again.\n");
                    continue;
                } else {
                    another_report_decision = decision;
                    break;
                }
            }
            if (another_report_decision == 'Y') continue;
            else if (another_report_decision == 'N') break;
        }

        if (choice == 2) {
            if (*number_of_lists < 1) {
                printf("\n\n------- No Reports Listed-------\n");
                continue;
            }
            printf("\n\n------- Lists Of Reports -------\n");
            printf("%-15s%-15s%-15s\n", "Flood Level", "Rain Intensity", "Located Zone");
            for (int i = 0; i < *number_of_lists; i++) {
                printf("%-15d%-15d%-15d\n", report_lists[i].flood, report_lists[i].rain, report_lists[i].zone);
            }
            continue;
        } else if (choice == 3) {
            fclose(fp_report);
            return;
        }
    }
    fclose(fp_report);
}

void PassageAnnouncement() {
    struct Zone zones[MAX_ZONES];
    struct reportstruct reports[MAX_REPORTS];
    int zoneCount, reportCount;

    loadZoneData(zones, &zoneCount);
    loadReportData(reports, &reportCount);

    printf("\n========== ZONE STATUS ANNOUNCEMENT ==========\n");

    for (int zone = 1; zone <= 3; zone++) {
        int houses = 0;
        for (int z = 0; z < zoneCount; z++) {
            if (zones[z].Zonenumber == zone) {
                houses = zones[z].houses;
                break;
            }
        }

        int floodCount[4] = {0}, rainCount[4] = {0}, totalReports = 0;
        for (int j = 0; j < reportCount; j++) {
            if (reports[j].zone == zone) {
                if (reports[j].flood >= 1 && reports[j].flood <= 3) floodCount[reports[j].flood]++;
                if (reports[j].rain >= 1 && reports[j].rain <= 3) rainCount[reports[j].rain]++;
                totalReports++;
            }
        }

        printf("\n--- ZONE %d ---\n", zone);
        printf("Residents: %d  |  Total Reports: %d\n", houses, totalReports);

        if (totalReports == 0) {
            printf("Status: GREEN ZONE\n");
            printf("Road Status: Passable\n");
            printf("highestFlood: None  |  Dominant Rain: None\n");
            printf("Est. Affected: 0\n");
            continue;
        }

        int highestFlood = 1;
        for (int i = 3; i >= 1; i--) {
            if (floodCount[i] > 0) {
                highestFlood = i;
                break;
            }
        }

        int domRain = 1;
        for (int i = 2; i <= 3; i++) {
            if (rainCount[i] > rainCount[domRain]) domRain = i;
        }

        float floodBase = (highestFlood == 1) ? 0.20f : (highestFlood == 2) ? 0.50f : 0.90f;
        float rainMod = (domRain == 1) ? -0.05f : (domRain == 2) ? 0.00f : 0.10f;
        float totalImpact = floodBase + rainMod;
        if (totalImpact < 0.10f) totalImpact = 0.10f;
        if (totalImpact > 1.00f) totalImpact = 1.00f;

        int estimatedAffected = (int)(houses * totalImpact);
        if (estimatedAffected < 1) estimatedAffected = 1;

        float ankleRatio = (houses > 0) ? (float)floodCount[1] / houses : 0.0f;
        float kneeRatio = (houses > 0) ? (float)floodCount[2] / houses : 0.0f;
        float waistRatio = (houses > 0) ? (float)floodCount[3] / houses : 0.0f;

        printf("Highest Flood: %s  |  Dominant Rain: %s\n",
               highestFlood == 1 ? "Ankle" : highestFlood == 2 ? "Knee" : "Waist",
               domRain == 1 ? "Light" : domRain == 2 ? "Moderate" : "Heavy");
        printf("Est. Affected: %d\n", estimatedAffected);
        printf("Ankle: %d (%.1f%%)  Knee: %d (%.1f%%)  Waist: %d (%.1f%%)\n",
               floodCount[1], ankleRatio * 100.0f, floodCount[2], kneeRatio * 100.0f, floodCount[3], waistRatio * 100.0f);

        int color = 0;
        if (ankleRatio >= ANKLE_YELLOW_PCT && color < 1) color = 1;

        if (floodCount[2] > 0) {
            if (kneeRatio >= KNEE_RED_PCT) color = 2;
            else if (kneeRatio >= KNEE_YELLOW_PCT && color < 1) color = 1;
        }

        if (floodCount[3] > 0) {
            if (waistRatio >= WAIST_RED_PCT) color = 2;
            else if (waistRatio >= WAIST_YELLOW_PCT && color < 1) color = 1;
        }

        if (color == 0) {
            printf("\nStatus: GREEN ZONE\n");
            printf("Road Status: Passable\n");
        } else if (color == 1) {
            printf("\nStatus: YELLOW ZONE\n");
            printf("Road Status: Passable with Caution\n");
            if (highestFlood == 1) printf("Note: All reports are ankle-level. Monitor for changes.\n");
            else if (highestFlood == 2) printf("Caution: Knee-level detected but not yet critical.\n");
            else printf("Warning: Waist-level detected. Prepare for possible evacuation.\n");
        } else {
            printf("\nStatus: RED ZONE\n");
            printf("Road Status: NOT PASSABLE\n");
            printf("Advisory: Evacuate if advised. Area is dangerous.\n");
        }
    }
}

// ===== HTTP SERVER FUNCTIONS =====

void send_json(SOCKET client, const char *json) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %zu\r\n"
        "\r\n%s", strlen(json), json);
    send(client, response, strlen(response), 0);
}

void send_error(SOCKET client, int code, const char *msg) {
    char response[512];
    snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n%s", code, msg, msg);
    send(client, response, strlen(response), 0);
}

void send_file(SOCKET client, const char *filename, const char *content_type) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        send_error(client, 404, "Not Found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *file_content = malloc(file_size + 1);
    fread(file_content, 1, file_size, fp);
    file_content[file_size] = '\0';
    fclose(fp);

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %ld\r\n"
        "\r\n", content_type, file_size);

    send(client, header, strlen(header), 0);
    send(client, file_content, file_size, 0);
    free(file_content);
}

void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

void handle_client(SOCKET client_socket, struct reportstruct *report_lists, int *number_of_lists) {
    char buffer[BUFFER_SIZE];
    int received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (received <= 0) {
        closesocket(client_socket);
        return;
    }
    buffer[received] = '\0';

    char method[8], path[256], protocol[16];
    sscanf(buffer, "%s %s %s", method, path, protocol);

    printf("[WEB] %s %s\n", method, path);

    // Serve static files
    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        send_file(client_socket, "index.html", "text/html");
    }
    else if (strcmp(path, "/style.css") == 0) {
        send_file(client_socket, "style.css", "text/css");
    }
    else if (strcmp(path, "/website.js") == 0) {
        send_file(client_socket, "website.js", "application/javascript");
    }

    // API: Get all reports
    else if (strcmp(path, "/api/reports") == 0 && strcmp(method, "GET") == 0) {
        char json[BUFFER_SIZE] = "{\"reports\":[";
        int first = 1;
        for (int i = 0; i < *number_of_lists; i++) {
            if (!first) strcat(json, ",");
            char item[128];
            snprintf(item, sizeof(item), "{\"flood\":%d,\"rain\":%d,\"zone\":%d}",
                     report_lists[i].flood, report_lists[i].rain, report_lists[i].zone);
            strcat(json, item);
            first = 0;
        }
        strcat(json, "]}");
        send_json(client_socket, json);
    }

    // API: Get zones
    else if (strcmp(path, "/api/zones") == 0 && strcmp(method, "GET") == 0) {
        struct Zone zones[MAX_ZONES];
        int zoneCount = 0;
        loadZoneData(zones, &zoneCount);

        char json[BUFFER_SIZE] = "{\"zones\":[";
        int first = 1;
        for (int i = 0; i < zoneCount; i++) {
            if (!first) strcat(json, ",");
            char item[128];
            snprintf(item, sizeof(item), "{\"zone\":%d,\"houses\":%d}",
                     zones[i].Zonenumber, zones[i].houses);
            strcat(json, item);
            first = 0;
        }
        strcat(json, "]}");
        send_json(client_socket, json);
    }

    // API: Get announcement (zone status)
    else if (strcmp(path, "/api/announcement") == 0 && strcmp(method, "GET") == 0) {
        struct Zone zones[MAX_ZONES];
        struct reportstruct reports[MAX_REPORTS];
        int zoneCount, reportCount;
        loadZoneData(zones, &zoneCount);
        loadReportData(reports, &reportCount);

        char json[BUFFER_SIZE] = "{\"announcement\":[";
        int first = 1;

        for (int zone = 1; zone <= 3; zone++) {
            int houses = 0;
            for (int z = 0; z < zoneCount; z++) {
                if (zones[z].Zonenumber == zone) {
                    houses = zones[z].houses;
                    break;
                }
            }

            int totalReports = 0;
            for (int j = 0; j < reportCount; j++) {
                if (reports[j].zone == zone) totalReports++;
            }

             if (totalReports == 0) {
                if (!first) strcat(json, ",");
                char item[512];
                snprintf(item, sizeof(item),
                    "{\"zone\":%d,\"status\":\"GREEN\",\"roadStatus\":\"Passable\",\"houses\":%d,"
                    "\"totalReports\":0,\"estimatedAffected\":0,\"highestFlood\":\"None\","
                    "\"dominantRain\":\"None\",\"impact\":0.00}",
                    zone, houses);
                strcat(json, item);
                first = 0;
                continue;
            }

            int floodCount[4] = {0}, rainCount[4] = {0}, totalReports = 0;
            for (int j = 0; j < reportCount; j++) {
                if (reports[j].zone == zone) {
                    if (reports[j].flood >= 1 && reports[j].flood <= 3) floodCount[reports[j].flood]++;
                    if (reports[j].rain >= 1 && reports[j].rain <= 3) rainCount[reports[j].rain]++;
                    totalReports++;
                }
            }

            int highestFlood = 1;
            for (int i = 3; i >= 1; i--) {
                if (floodCount[i] > 0) {
                    highestFlood = i;
                    break;
                }
            }

            int domRain = 1;
            for (int i = 2; i <= 3; i++) {
                if (rainCount[i] > rainCount[domRain]) domRain = i;
            }

            float floodBase = (highestFlood == 1) ? 0.20f : (highestFlood == 2) ? 0.50f : 0.90f;
            float rainMod = (domRain == 1) ? -0.05f : (domRain == 2) ? 0.00f : 0.10f;
            float totalImpact = floodBase + rainMod;
            if (totalImpact < 0.10f) totalImpact = 0.10f;
            if (totalImpact > 1.00f) totalImpact = 1.00f;

            int estimatedAffected = (int)(houses * totalImpact);
            if (estimatedAffected < 1) estimatedAffected = 1;

            float ankleRatio = (houses > 0) ? (float)floodCount[1] / houses : 0.0f;
            float kneeRatio = (houses > 0) ? (float)floodCount[2] / houses : 0.0f;
            float waistRatio = (houses > 0) ? (float)floodCount[3] / houses : 0.0f;

            int color = 0;
            if (ankleRatio >= ANKLE_YELLOW_PCT && color < 1) color = 1;
            if (floodCount[2] > 0) {
                if (kneeRatio >= KNEE_RED_PCT) color = 2;
                else if (kneeRatio >= KNEE_YELLOW_PCT && color < 1) color = 1;
            }
            if (floodCount[3] > 0) {
                if (waistRatio >= WAIST_RED_PCT) color = 2;
                else if (waistRatio >= WAIST_YELLOW_PCT && color < 1) color = 1;
            }

            const char *status = (color == 0) ? "GREEN" : (color == 1) ? "YELLOW" : "RED";
            const char *roadStatus = (color == 0) ? "Passable" : (color == 1) ? "Passable with Caution" : "NOT PASSABLE";
            const char *highestFloodStr = (highestFlood == 1) ? "Ankle" : (highestFlood == 2) ? "Knee" : "Waist";
            const char *domRainStr = (domRain == 1) ? "Light" : (domRain == 2) ? "Moderate" : "Heavy";

            if (!first) strcat(json, ",");
            char item[512];
            snprintf(item, sizeof(item),
                "{\"zone\":%d,\"status\":\"%s\",\"roadStatus\":\"%s\",\"houses\":%d,"
                "\"totalReports\":%d,\"estimatedAffected\":%d,\"highestFlood\":\"%s\","
                "\"dominantRain\":\"%s\",\"impact\":%.2f}",
                zone, status, roadStatus, houses, totalReports, estimatedAffected,
                highestFloodStr, domRainStr, totalImpact);
            strcat(json, item);
            first = 0;
        }
        strcat(json, "]}");
        send_json(client_socket, json);
    }

    // API: Submit report (with overwrite logic)
    else if (strcmp(path, "/api/report") == 0 && strcmp(method, "POST") == 0) {
        // Find body (after \r\n\r\n)
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) body += 4;

        int flood = 0, rain = 0, zone = 0;
        if (body) {
            sscanf(body, "{\"flood\":%d,\"rain\":%d,\"zone\":%d}", &flood, &rain, &zone);
        }

        if (flood < 1 || flood > 3 || rain < 1 || rain > 3 || zone < 1 || zone > 3) {
            send_error(client_socket, 400, "Invalid data");
            closesocket(client_socket);
            return;
        }

        // ZONE LIMIT CHECK (same as console)
        struct Zone zones[MAX_ZONES];
        int zoneCount = 0;
        loadZoneData(zones, &zoneCount);

        int zoneHouses = 0;
        for (int z = 0; z < zoneCount; z++) {
            if (zones[z].Zonenumber == zone) {
                zoneHouses = zones[z].houses;
                break;
            }
        }

        int zoneReportCount = 0;
        for (int i = 0; i < *number_of_lists; i++) {
            if (report_lists[i].zone == zone) zoneReportCount++;
        }

        int zoneReportLimit = zoneHouses;
        if (zoneReportLimit < 1) zoneReportLimit = 1;

        // OVERWRITE LOGIC
        if (zoneReportCount >= zoneReportLimit) {
            int oldestIndex = -1;
            for (int i = 0; i < *number_of_lists; i++) {
                if (report_lists[i].zone == zone) {
                    oldestIndex = i;
                    break;
                }
            }
            if (oldestIndex != -1) {
                report_lists[oldestIndex].flood = flood;
                report_lists[oldestIndex].rain = rain;
                report_lists[oldestIndex].zone = zone;
                rewriteReportFile(report_lists, *number_of_lists);
                send_json(client_socket, "{\"success\":true,\"message\":\"Report overwritten\",\"overwritten\":true}");
                closesocket(client_socket);
                return;
            }
        }

        // NORMAL SAVE
        report_lists[*number_of_lists].flood = flood;
        report_lists[*number_of_lists].rain = rain;
        report_lists[*number_of_lists].zone = zone;
        (*number_of_lists)++;

        FILE *fp = fopen("report.txt", "a");
        if (fp) {
            fprintf(fp, "%d %d %d\n", flood, rain, zone);
            fclose(fp);
        }

        send_json(client_socket, "{\"success\":true,\"message\":\"Report saved\",\"overwritten\":false}");
    }

    // API: Update zone (admin)
    else if (strcmp(path, "/api/zone") == 0 && strcmp(method, "POST") == 0) {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) body += 4;

        char username[30] = "", password[30] = "";
        int targetZone = 0, newHouses = 0;

        // Simple JSON parsing
        sscanf(body, "{\"zone\":%d,\"houses\":%d,\"username\":\"%29[^\"]\",\"password\":\"%29[^\"]\"}",
               &targetZone, &newHouses, username, password);

        if (strcmp(username, "admin") != 0 || strcmp(password, "1234") != 0) {
            send_json(client_socket, "{\"success\":false,\"error\":\"Access Denied\"}");
            closesocket(client_socket);
            return;
        }

        FILE *fp = fopen("zone.txt", "r");
        FILE *temp = fopen("temp.txt", "w");
        if (!fp || !temp) {
            send_json(client_socket, "{\"success\":false,\"error\":\"File error\"}");
            if (fp) fclose(fp);
            if (temp) fclose(temp);
            closesocket(client_socket);
            return;
        }

        struct Zone z;
        int found = 0;
        while (fscanf(fp, "%d %d", &z.Zonenumber, &z.houses) == 2) {
            if (z.Zonenumber == targetZone) {
                z.houses = newHouses;
                found = 1;
            }
            fprintf(temp, "%d %d\n", z.Zonenumber, z.houses);
        }
        fclose(fp);
        fclose(temp);

        remove("zone.txt");
        rename("temp.txt", "zone.txt");

        send_json(client_socket, found ? "{\"success\":true}" : "{\"success\":false,\"error\":\"Zone not found\"}");
    }

    else {
        send_error(client_socket, 404, "Not Found");
    }

    closesocket(client_socket);
}

void start_web_server(struct reportstruct *report_lists, int *number_of_lists) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("❌ Failed to create socket\n");
        return;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("❌ Bind failed\n");
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        printf("❌ Listen failed\n");
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    printf("\n🌐 Web server started at http://localhost:%d\n", PORT);
    printf("   Open your browser to use the web interface\n");
    printf("   (Console app still running - choose option 1-5)\n\n");

    // Set non-blocking for console + server
    u_long mode = 1;
    ioctlsocket(server_socket, FIONBIO, &mode);

    while (1) {
        SOCKET client_socket = accept(server_socket, NULL, NULL);
        if (client_socket != INVALID_SOCKET) {
            handle_client(client_socket, report_lists, number_of_lists);
        }
        Sleep(10); // Small delay to prevent CPU spinning
    }

    closesocket(server_socket);
    WSACleanup();
}

int main() {
    int targetZone, newHouses;
    int choice = 0;
    struct Zone zonesData;
    struct Zone zones[MAX_ZONES];
    struct Admin official;

    int number_of_report_lists;
    struct reportstruct *report_lists;
    report_lists = (struct reportstruct *)malloc(sizeof(struct reportstruct) * MAX_REPORTS);
    if (report_lists == NULL) {
        printf("Memory allocation failed. Discontinuing program...");
        return 1;
    }

    if (read_report_file(report_lists, &number_of_report_lists) == 1) {
        printf("Discontinuing program...");
        free(report_lists);
        return 0;
    }

    // Initialize zone.txt
    FILE *check = fopen("zone.txt", "r");
    if (check == NULL) {
        FILE *create = fopen("zone.txt", "w");
        if (create != NULL) {
            fprintf(create, "1 0\n2 0\n3 0\n");
            fclose(create);
        }
    } else {
        fseek(check, 0, SEEK_END);
        if (ftell(check) == 0) {
            fclose(check);
            FILE *create = fopen("zone.txt", "w");
            if (create != NULL) {
                fprintf(create, "1 0\n2 0\n3 0\n");
                fclose(create);
            }
        } else {
            fclose(check);
        }
    }

    // Start web server in background thread
    HANDLE webThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_web_server, report_lists, 0, NULL);
    if (webThread) {
        printf("🚀 Web server thread started!\n");
    }

    // Console menu
    do {
        system("cls");
        printf("1. Report\n2. Update Zone\n3. View Zone Data\n4. Announcement\n5. Exit\nChoice: ");
        scanf("%d", &choice);
        while (getchar() != '\n');

        switch (choice) {
            case 1: {
                reporting(report_lists, &number_of_report_lists);
                break;
            }
            case 2: {
                system("cls");
                printf("Enter Username: ");
                scanf(" %29[^\n]", official.username);
                printf("Enter Password: ");
                scanf(" %29[^\n]", official.password);

                if (strcmp(official.username, "admin") != 0 || strcmp(official.password, "1234") != 0) {
                    printf("\nAccess Denied!");
                    pauseScreen();
                    break;
                }

                system("cls");
                printf("1. Zone 1\n2. Zone 2\n3. Zone 3");
                printf("\nEnter Zone Number: ");
                while (scanf("%d", &targetZone) != 1 || targetZone < 1 || targetZone > 3) {
                    while (getchar() != '\n');
                    printf("\nInvalid Zone\nEnter Again: ");
                }
                while (getchar() != '\n');

                printf("Enter Number of Houses: ");
                while (scanf("%d", &newHouses) != 1 || newHouses < 0) {
                    while (getchar() != '\n');
                    printf("\nInvalid Number of Houses\nEnter Again: ");
                }
                while (getchar() != '\n');

                FILE *fp = fopen("zone.txt", "r");
                FILE *temp = fopen("temp.txt", "w");
                if (fp == NULL || temp == NULL) {
                    printf("\nFile Error!");
                    if (fp) fclose(fp);
                    if (temp) fclose(temp);
                    pauseScreen();
                    break;
                }

                int found = 0;
                while (fscanf(fp, "%d %d", &zonesData.Zonenumber, &zonesData.houses) == 2) {
                    if (zonesData.Zonenumber == targetZone) {
                        zonesData.houses = newHouses;
                        found = 1;
                    }
                    fprintf(temp, "%d %d\n", zonesData.Zonenumber, zonesData.houses);
                }
                fclose(fp);
                fclose(temp);
                remove("zone.txt");
                rename("temp.txt", "zone.txt");

                if (found) printf("\nZone %d updated successfully!", targetZone);
                else printf("\nZone not found");
                pauseScreen();
                break;
            }
            case 3: {
                system("cls");
                FILE *fp = fopen("zone.txt", "r");
                if (fp == NULL) {
                    printf("\nNo zone data found!");
                    pauseScreen();
                    break;
                }
                int i = 0;
                printf("\n--- ZONE DATA ---\n");
                while (fscanf(fp, "%d %d", &zones[i].Zonenumber, &zones[i].houses) != EOF && i < MAX_ZONES) {
                    printf("Zone %d - %d houses\n", zones[i].Zonenumber, zones[i].houses);
                    i++;
                }
                fclose(fp);
                pauseScreen();
                break;
            }
            case 4: {
                PassageAnnouncement();
                pauseScreen();
                break;
            }
            case 5: {
                printf("\nGoodbye!");
                break;
            }
            default:
                printf("\nInvalid choice!");
                pauseScreen();
                break;
        }
    } while (choice != 5);

    free(report_lists);
    return 0;
}