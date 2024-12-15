#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ncurses.h>

#define LINE_LENGTH 512
#define MAX_RESULTS 200

typedef struct {
    char *data;
    size_t size;
} Response;

size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res) {
    size_t new_len = res->size + size * nmemb;
    char *temp = realloc(res->data, new_len + 1);
    if (!temp) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 0;
    }
    res->data = temp;
    memcpy(res->data + res->size, ptr, size * nmemb);
    res->size = new_len;
    res->data[new_len] = '\0';
    return size * nmemb;
}

void cleanup_ncurses() {
    if (!isendwin()) {
        endwin();
    }
}

int search_aur(const char *query, char results[][LINE_LENGTH]) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    char url[256];
    snprintf(url, sizeof(url), "https://aur.archlinux.org/rpc/?v=5&type=search&arg=%s", query);

    Response response = {NULL, 0};
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response.data == NULL) {
        free(response.data);
        return 0;
    }

    int count = 0;
    char *ptr = response.data;
    while ((ptr = strstr(ptr, "\"Name\":\"")) && count < MAX_RESULTS) {
        ptr += 8;
        char package_name[128] = {0};
        sscanf(ptr, "%127[^\"]", package_name);
        snprintf(results[count], LINE_LENGTH, "AUR: %s", package_name);
        count++;
    }

    free(response.data);
    return count;
}

void show_results_ncurses(char results[][LINE_LENGTH], int total) {
    int highlight = 0, ch;
    int start = 0;
    int max_lines = LINES - 4;

    while (1) {
        clear();
        mvprintw(1, 2, "Results (UP/DOWN to navigate, ENTER to install, ESC to return):");
        mvprintw(2, 2, "Found %d results.", total);

        for (int i = 0; i < max_lines && (start + i) < total; i++) {
            if ((start + i) == highlight) {
                attron(A_REVERSE);
                mvprintw(i + 4, 2, "%s", results[start + i]);
                attroff(A_REVERSE);
            } else {
                mvprintw(i + 4, 2, "%s", results[start + i]);
            }
        }

        refresh();
        ch = getch();

        switch (ch) {
            case KEY_UP:
                if (highlight > 0) highlight--;
                if (highlight < start) start--;
                break;
            case KEY_DOWN:
                if (highlight < total - 1) highlight++;
                if (highlight >= start + max_lines) start++;
                break;
            case 27:
                return;
            case 10: {
                char package_name[128];
                sscanf(results[highlight], "%*s %127s", package_name);
                char command[256];
                if (strstr(results[highlight], "AUR")) {
                    snprintf(command, sizeof(command), "yay -S --noconfirm %s", package_name);
                } else {
                    snprintf(command, sizeof(command), "sudo pacman -S --noconfirm %s", package_name);
                }
                cleanup_ncurses();
                printf("Installing: %s...\n", package_name);
                system(command);
                return;
            }
        }
    }
}

void show_home_screen() {
    char query[128];
    char results[MAX_RESULTS][LINE_LENGTH];
    int total = 0;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    while (1) {
        clear();
        mvprintw(LINES / 2 - 1, COLS / 2 - 20, "Enter package name to search (or ESC to quit): ");
        echo();
        mvgetnstr(LINES / 2, COLS / 2 - 20, query, sizeof(query) - 1);
        noecho();

        if (strlen(query) == 0 || query[0] == '\n') {
            cleanup_ncurses();
            printf("No input provided. Exiting...\n");
            return;
        }

        clear();
        printw("[INFO] Searching for '%s'. Please wait...\n", query);
        refresh();

        total = search_aur(query, results);

        if (total == 0) {
            mvprintw(LINES / 2, COLS / 2 - 20, "No results found for '%s'. Press any key to continue.", query);
            getch();
        } else {
            show_results_ncurses(results, total);
        }
    }

    cleanup_ncurses();
}

int main() {
    show_home_screen();
    return 0;
}

