#define NCURSES_WIDECHAR 1
#define _XOPEN_SORCE 700

#include "./libsql.h"
#include <ncurses.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#define MAX_FLASHCARDS 10000
#define MAX_SEARCH_LEN 20

struct database {
        libsql_database_t db;
        libsql_connection_t connection;
};

struct flashcard {
        int id;
        char const *term;
        char const *definition;
        int confidence;
};

void init_ncurses() {
        initscr();

        // initilise colours

        start_color();

        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);

        keypad(stdscr, TRUE);
        nonl();
        cbreak();
        noecho();
}

struct database init_database(char *filename) {
        struct database db;

        libsql_setup((libsql_config_t){0});

        db.db =
            libsql_database_init((libsql_database_desc_t){.path = filename});

        db.connection = libsql_database_connect(db.db);

        return db;
}

libsql_rows_t query(struct database db, char *statement) {
        return libsql_statement_query(
            libsql_connection_prepare(db.connection, statement));
}

void cleanup_ncurses() {
        endwin();
        resetty();
        reset_shell_mode();
}

void mvaddslicec(int y, int x, char const *buffer, int len) {
        for (int i = 0; i <= len - 1; i++) {
                mvaddch(y, x + i - len / 2, buffer[i]);
        }
}

void mvaddslice(int y, int x, char const *buffer, int len) {
        for (int i = 0; i <= len - 1; i++) {
                mvaddch(y, x + i, buffer[i]);
        }
}

int mvprintdata(int y, int x, char const *buffer) {
        int lines = 1;
        for (int i = 0; i < strlen(buffer) - 1; i++) {
                if (buffer[i] == '\\' && buffer[i + 1] == 'n') {
                        mvaddslicec(y - 1 + lines, x, buffer, i);
                        lines += mvprintdata(y + lines, x, buffer + 2 + i);
                        break;
                }
                if (buffer[i + 2] == '\0') {
                        mvaddslicec(y + lines - 1, x, buffer, strlen(buffer));
                }
        }
        return lines;
}

int clamp(int min, int value, int max) {
        if (min > value) {
                return min;
        } else if (max < value) {
                return max;
        } else {
                return value;
        }
}

void update_confidence(struct database db, int flashcard_id, int confidence) {
        char confidence_string[2];
        confidence = clamp(0, confidence, 5);
        snprintf(confidence_string, 2, "%1d", confidence);
        mvaddstr(getmaxy(stdscr) / 2 - 1, getmaxx(stdscr) / 2 + 8,
                 confidence_string);
        char stmt[84];
        snprintf(stmt, 84,
                 "UPDATE flashcard SET confidence = %d WHERE "
                 "flashcard.flashcardid = %d",
                 confidence, flashcard_id);
        query(db, stmt);
        libsql_database_sync(db.db);
}

int random_card_index(struct flashcard *card_list, int card_count) {
        int total = 0;
        for (int i = 0; i < card_count; i++) {
                total += (5 - card_list[i].confidence);
        }

        int confidence_index = random() % total;
        int remainder = confidence_index;

        for (int i = 0; i < card_count; i++) {
                remainder -= 5 - card_list[i].confidence;
                if (remainder <= 0) {
                        return i;
                }
        }
        return 0;
}

struct flashcard_set {
        int setid;
        char const *subject;
        char const *topic;
};

struct card_set_relation {
        int setid;
        int cardid;
};

void filter_cards(struct database db, struct flashcard *cardlist,
                  char *topic_filter, int card_count, char *subject_filter,
                  int index_list[MAX_FLASHCARDS]) {
        regex_t subject_regex;
        regcomp(&subject_regex, subject_filter, 0);
        regex_t topic_regex;
        regcomp(&topic_regex, topic_filter, 0);

        struct flashcard_set set_list[MAX_FLASHCARDS];
        libsql_rows_t subjects = query(db, "SELECT subject FROM flashcardset");
        libsql_row_t subject_row;
        libsql_result_value_t set_id;
        libsql_result_value_t subject;
        libsql_result_value_t topic;

        for (int i = 0; (!set_id.err && !subject.err && !topic.err); i++) {
                subject_row = libsql_rows_next(subjects);
                set_id = libsql_row_value(subject_row, 0);
                subject = libsql_row_value(subject_row, 1);
                topic = libsql_row_value(subject_row, 2);
                set_list[i].setid = set_id.ok.value.integer;
                set_list[i].subject = subject.ok.value.text.ptr;
                set_list[i].topic = topic.ok.value.text.ptr;
        }

        int sets[MAX_FLASHCARDS];
        int set_num = 0;

        for (int i = 0; i <= card_count; i++) {
                if (regexec(&subject_regex, set_list[i].subject, 0, NULL, 0) ||
                    regexec(&topic_regex, set_list[i].topic, 0, NULL, 0)) {
                        sets[set_num] = set_list[i].setid;
                        set_num++;
                }
        }

        getch();

        struct card_set_relation card_in_set[MAX_FLASHCARDS];
        libsql_rows_t card_ids = query(db, "SELECT * FROM cardinset");
        libsql_row_t card_row = libsql_rows_next(card_ids);
        libsql_result_value_t card = libsql_row_value(card_row, 1);
        libsql_result_value_t set = libsql_row_value(card_row, 1);

        for (int i = 0; !(set.err || card.err); i++) {
                card_in_set[i].cardid = card.ok.value.integer;
                card_in_set[i].setid = set.ok.value.integer;
                card_row = libsql_rows_next(card_ids);
                card = libsql_row_value(card_row, 1);
                set = libsql_row_value(card_row, 1);
        }

        for (int i, j, k = 0; i <= card_count; i++) {
                if (cardlist[i].id == card_in_set[j].cardid) {
                        index_list[k] = cardlist[i].id;
                        k++;
                } else if (i == card_count && j != set_num) {
                        i = -1;
                        j++;
                } else {
                        break;
                }
        }
}

int main(int argc, char **argv) {
        srandom(time(NULL));

        if (argc < 2) {
                printf("provide a filename!\n");
                init_ncurses();
                refresh();
                cleanup_ncurses();
                return EXIT_FAILURE;
        }

        init_ncurses();
        refresh();

        struct database db = init_database(argv[1]);

        move(getmaxy(stdscr) / 2, getmaxx(stdscr) / 2);

        libsql_rows_t rows = query(db, "SELECT * FROM flashcard");

        int card_num = 0;

        struct flashcard
            card_list[MAX_FLASHCARDS]; // MAX_FLASHCARDS rows as that is the
                                       // maximum number of flashcards

        char counter[6];

        // Read flashcard list
        libsql_row_t rw;
        int card_count;

        mvaddstr(getmaxy(stdscr) / 2, getmaxx(stdscr) / 2 - 10,
                 "Reading flashcards...");

        for (int i = 0; i < MAX_FLASHCARDS; i++) {
                rw = libsql_rows_next(rows);

                libsql_result_value_t id = libsql_row_value(rw, 0);
                libsql_result_value_t term = libsql_row_value(rw, 1);
                libsql_result_value_t definition = libsql_row_value(rw, 2);
                libsql_result_value_t confidence = libsql_row_value(rw, 3);

                if (id.err || term.err || definition.err || confidence.err) {
                        card_count = i;
                        break;
                } else {

                        card_list[i].id = id.ok.value.integer;
                        card_list[i].term = term.ok.value.text.ptr;
                        card_list[i].definition = definition.ok.value.text.ptr;
                        card_list[i].confidence = confidence.ok.value.integer;

                        snprintf(counter, 6, "%d", i + 1);
                        mvaddstr(1, 1, counter);
                }
        }

        clear();
        box(stdscr, 0, 0);
        mvaddstr(getmaxy(stdscr) / 2, getmaxx(stdscr) / 2 - 10,
                 "Read all flashcards!");
        mvaddstr(1, 1, counter);
        getch();

        struct flashcard card = card_list[card_num];

        _Bool show_def = FALSE;
        _Bool search_mode = FALSE;

        int filtered_indecies[MAX_FLASHCARDS];
        int filter_index = 0;

        char character;

        while (card_num <= card_count + 1) {
                clear();
                box(stdscr, 0, 0);

                char flashcard_id_string[5];
                char confidence_string[2];

                if (card_num == card_count) {
                        mvprintdata(getmaxy(stdscr) / 2, getmaxx(stdscr) / 2,
                                    "Are you sure you want to finish?\\nY/N");
                } else {
                        snprintf(flashcard_id_string, 5, "%4d",
                                 card_list[card_num].id);
                        snprintf(confidence_string, 2, "%1d",
                                 card_list[card_num].confidence);

                        mvaddstr(getmaxy(stdscr) / 2 - 2,
                                 getmaxx(stdscr) / 2 - 9, "Flashcard ID: ");
                        mvaddstr(getmaxy(stdscr) / 2 - 2,
                                 getmaxx(stdscr) / 2 + 5, flashcard_id_string);

                        mvaddstr(getmaxy(stdscr) / 2 - 1,
                                 getmaxx(stdscr) / 2 - 7, "Confidence: ");
                        mvaddstr(getmaxy(stdscr) / 2 - 1,
                                 getmaxx(stdscr) / 2 + 8, confidence_string);

                        int offset = mvprintdata(getmaxy(stdscr) / 2 + 1,
                                                 getmaxx(stdscr) / 2,
                                                 card_list[card_num].term);

                        if (show_def) {
                                mvprintdata(getmaxy(stdscr) / 2 + 2 + offset,
                                            getmaxx(stdscr) / 2,
                                            card_list[card_num].definition);
                        }
                }
                move(0, 0);

                refresh();

                if (!search_mode) {
                        character = getch();
                }
                int old_num = card_num;
                _Bool manage_confidence = card_num != card_count;
                if (manage_confidence && !search_mode) {
                        switch (character) {
                        case 'q':
                                cleanup_ncurses();
                                return EXIT_SUCCESS;
                        case '0':
                                update_confidence(db, card_list[card_num].id,
                                                  0);
                                card_list[card_num].confidence = 0;
                                break;
                        case '1':
                                update_confidence(db, card_list[card_num].id,
                                                  1);
                                card_list[card_num].confidence = 1;
                                break;
                        case '2':
                                update_confidence(db, card_list[card_num].id,
                                                  2);
                                card_list[card_num].confidence = 2;
                                break;
                        case '3':
                                update_confidence(db, card_list[card_num].id,
                                                  3);
                                card_list[card_num].confidence = 3;
                                break;
                        case '4':
                                update_confidence(db, card_list[card_num].id,
                                                  4);
                                card_list[card_num].confidence = 4;
                                break;
                        case '5':
                                update_confidence(db, card_list[card_num].id,
                                                  5);
                                card_list[card_num].confidence = 5;
                                break;
                        case 'j':
                                update_confidence(
                                    db, card_list[card_num].id,
                                    card_list[card_num].confidence - 1);
                                card_list[card_num].confidence = clamp(
                                    0, card_list[card_num].confidence - 1, 5);
                                break;
                        case 'k':
                                update_confidence(
                                    db, card_list[card_num].id,
                                    card_list[card_num].confidence + 1);
                                card_list[card_num].confidence = clamp(
                                    0, card_list[card_num].confidence + 1, 5);
                                break;
                        case 'l':
                                card_num = clamp(0, card_num + 1, card_count);
                                show_def = FALSE;
                                break;
                        case 'h':
                                card_num = clamp(0, card_num - 1, card_count);
                                show_def = FALSE;
                                break;
                        case ' ':
                                show_def = !show_def;
                                break;
                        case 'r':
                                card_num = clamp(
                                    0, random_card_index(card_list, card_count),
                                    card_count - 1);
                                break;
                        case '/':
                                search_mode = TRUE;
                                break;
                        case 'n':
                                filter_index++;
                                card_num = filtered_indecies[filter_index];
                                break;
                        }
                } else if (manage_confidence && search_mode) {
                        char input_char;
                        char subject[MAX_SEARCH_LEN];
                        int subject_index = 0;
                        char topic[MAX_SEARCH_LEN];
                        int topic_index = 0;
                        mvaddstr(1, 1, "Searching...");
                        mvaddstr(2, 1, "|- Subject:");
                        mvaddstr(3, 1, "|- Topic  :");
                        move(2, 13);
                        echo();
                        while (true) {
                                input_char = getch();
                                if (input_char == '\b') {
                                        if (getcury(stdscr) == 2) {
                                                mvaddch(2, 13 + subject_index,
                                                        ' ');
                                                subject[subject_index] = '\0';
                                                subject[subject_index - 1] =
                                                    '\0';
                                                subject_index--;
                                        } else if (getcury(stdscr) == 3) {
                                                topic[topic_index] = '\0';
                                                topic[topic_index + 1] = '\0';
                                                topic_index--;
                                        }
                                } else if (input_char == '\r') {
                                        if (getcury(stdscr) == 2) {
                                                subject[subject_index] = '\0';
                                                mvaddstr(10, 10,
                                                         "taken subject input");
                                                mvaddslice(11, 11, subject,
                                                           subject_index);
                                                move(3, 13);
                                        } else if (getcury(stdscr) == 3) {
                                                topic[topic_index] = '\0';
                                                mvaddstr(12, 10,
                                                         "taken topic input");
                                                mvaddslice(13, 11, topic,
                                                           topic_index);
                                                break;
                                        }
                                } else {
                                        if (getcury(stdscr) == 2) {
                                                subject[subject_index] =
                                                    input_char;
                                                subject_index++;
                                        } else if (getcury(stdscr) == 3) {
                                                topic[topic_index] = input_char;
                                                topic_index++;
                                        }
                                }
                        }
                        mvaddstr(14, 10, "beginning search...");
                        int card_indecies[MAX_FLASHCARDS];
                        filter_cards(db, card_list, topic, card_count, subject,
                                     card_indecies);
                        search_mode = FALSE;
                } else {
                        if (character == 'q' || character == 'y' ||
                            character == 'Y') {
                                cleanup_ncurses();
                                return EXIT_SUCCESS;
                        } else if (character == 'n' || character == 'N' ||
                                   character == 'h') {
                                card_num--;
                        }
                }
                move(0, 0);
        }

        cleanup_ncurses();

        return EXIT_SUCCESS;
}
