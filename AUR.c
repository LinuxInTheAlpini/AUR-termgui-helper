#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ncurses.h>
#include <unistd.h>

#define CONFIG_FILE "./aur"

typedef struct {
    char *data;
    size_t size;
} Response;

typedef struct {
    char name[128];
    char description[256];
} Package;

// Callback per cURL
size_t write_callback(void *ptr, size_t size, size_t nmemb, Response *res) {
    size_t new_len = res->size + size * nmemb;
    res->data = realloc(res->data, new_len + 1);
    if (res->data == NULL) {
        fprintf(stderr, "Memory allocation error!\n");
        exit(EXIT_FAILURE);
    }
    memcpy(res->data + res->size, ptr, size * nmemb);
    res->size = new_len;
    res->data[res->size] = '\0';
    return size * nmemb;
}

// Gestisce i temi
void init_theme(int theme) {
    start_color();
    switch (theme) {
        case 1: // Classico
            init_pair(1, COLOR_WHITE, COLOR_BLACK);
            init_pair(2, COLOR_CYAN, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            break;
        case 2: // Cielo
            init_pair(1, COLOR_BLUE, COLOR_WHITE);
            init_pair(2, COLOR_CYAN, COLOR_WHITE);
            init_pair(3, COLOR_BLACK, COLOR_WHITE);
            break;
        case 3: // Notte
            init_pair(1, COLOR_GREEN, COLOR_BLACK);
            init_pair(2, COLOR_YELLOW, COLOR_BLACK);
            init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
            break;
        default:
            init_pair(1, COLOR_WHITE, COLOR_BLACK);
    }
}

// Salva il tema selezionato nel file di configurazione
void save_theme(int theme) {
    FILE *file = fopen(CONFIG_FILE, "w");
    if (file == NULL) {
        perror("Failed to save theme");
        return;
    }
    fprintf(file, "%d\n", theme);
    fclose(file);
}

// Carica il tema dal file di configurazione
int load_theme() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (file == NULL) {
        return 0; // Default: nessun tema salvato
    }
    int theme;
    if (fscanf(file, "%d", &theme) != 1) {
        fclose(file);
        return 0; // Default: errore nella lettura
    }
    fclose(file);
    return theme;
}

// Menu per selezionare il tema
int select_theme() {
    int choice = 0;
    int ch;

    while (1) {
        clear();
        mvprintw(0, 0, "Select a theme for the interface:");
        mvprintw(2, 2, "1. Classic (White on Black)");
        mvprintw(3, 2, "2. Sky (Blue on White)");
        mvprintw(4, 2, "3. Night (Green on Black)");
        mvprintw(6, 0, "Press the number of your choice (1-3): ");

        ch = getch();
        if (ch == '1' || ch == '2' || ch == '3') {
            choice = ch - '0';
            save_theme(choice); // Salva il tema selezionato
            break;
        }
    }
    return choice;
}

// Cerca pacchetti AUR
char *search_aur(const char *query) {
    CURL *curl;
    CURLcode res;
    Response response = {NULL, 0};
    char url[256];

    snprintf(url, sizeof(url), "https://aur.archlinux.org/rpc/?v=5&type=search&arg=%s", query);

    curl = curl_easy_init();
    if (!curl) return NULL;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        free(response.data);
        return NULL;
    }

    curl_easy_cleanup(curl);
    return response.data;
}

// Parsing del JSON
int parse_results(const char *json, Package packages[50]) {
    int count = 0;
    const char *ptr = json;

    while ((ptr = strstr(ptr, "\"Name\":\"")) && count < 50) {
        ptr += 8;
        sscanf(ptr, "%127[^\"]", packages[count].name);

        const char *desc_ptr = strstr(ptr, "\"Description\":\"");
        if (desc_ptr) {
            desc_ptr += 15;
            sscanf(desc_ptr, "%255[^\"]", packages[count].description);
        } else {
            strncpy(packages[count].description, "N/A", sizeof(packages[count].description));
        }
        count++;
    }
    return count;
}

// Funzione per installare un pacchetto con yay
void install_package(const char *package_name) {
    char command[512];

    snprintf(command, sizeof(command), "yay --noconfirm --needed -S %s", package_name);
    clear();
    mvprintw(0, 0, "Installing %s... Please wait.\n", package_name);
    refresh();

    int result = system(command);
    if (result == 0) {
        mvprintw(2, 0, "Installation of %s completed successfully! Press any key to continue...", package_name);
    } else {
        mvprintw(2, 0, "Installation of %s failed! Press any key to continue...", package_name);
    }
    getch();
}

// Menu di selezione dei risultati
void display_results(Package packages[50], int count) {
    int selected = 0;
    int ch;

    while (1) {
        clear();
        attron(COLOR_PAIR(3));
        mvprintw(0, 0, "Results: Use UP/DOWN to navigate, ENTER to install, Q to quit");
        attroff(COLOR_PAIR(3));

        for (int i = 0; i < count; i++) {
            if (i == selected) {
                attron(COLOR_PAIR(2));
                mvprintw(2 + i, 2, "%s - %s", packages[i].name, packages[i].description);
                attroff(COLOR_PAIR(2));
            } else {
                mvprintw(2 + i, 2, "%s - %s", packages[i].name, packages[i].description);
            }
        }

        ch = getch();
        switch (ch) {
            case KEY_UP:
                selected = (selected > 0) ? selected - 1 : count - 1;
                break;
            case KEY_DOWN:
                selected = (selected < count - 1) ? selected + 1 : 0;
                break;
            case '\n': // ENTER
                install_package(packages[selected].name);
                return;
            case 'q':
                return;
        }
    }
}

// GUI principale
void search_gui() {
    char query[128];
    Package packages[50];
    int package_count = 0;

    while (1) {
        clear();
        echo();
        attron(COLOR_PAIR(1));
        mvprintw(0, 0, "Enter a package name to search (or 'q' to quit): ");
        attroff(COLOR_PAIR(1));
        getnstr(query, sizeof(query) - 1);
        noecho();

        if (strcmp(query, "q") == 0) {
            break;
        }

        char *result = search_aur(query);
        if (!result) {
            mvprintw(2, 0, "Error fetching results! Press any key to continue...");
            getch();
            continue;
        }

        package_count = parse_results(result, packages);
        free(result);

        if (package_count == 0) {
            mvprintw(2, 0, "No results found. Press any key to continue...");
            getch();
        } else {
            display_results(packages, package_count);
        }
    }
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int theme = load_theme(); // Carica il tema salvato
    if (theme < 1 || theme > 3) {
        theme = select_theme(); // Mostra il menu se non c'è un tema valido
    }
    init_theme(theme); // Applica il tema

    search_gui();

    endwin();
    return 0;
}

