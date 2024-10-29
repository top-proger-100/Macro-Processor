#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Item {
    char* name;
    int stringsLength;
    char** strings;
} typedef Item;

int compareFunc(const void* a, const void* b ) {
    return strcmp((*(Item*)a).name, (*(Item*)b).name);
}

void replace(char* str, char* substrFrom, char* substrTo) {
    size_t ssl = strlen(substrFrom),
      rpl = strlen(substrTo);
    char *p = strstr(str, substrFrom);
    if (p) {
        if (rpl > ssl) {
            // Перемещаем остаток строки вправо, освобождая место для новой подстроки
            memmove(p + rpl, p + ssl, strlen(p + ssl) + 1);
        }
        // Копируем новую подстроку на место старой
        memcpy(p, substrTo, rpl);
        // Если новая подстрока короче старой, сдвигаем строку влево
        if (rpl < ssl) {
            memmove(p + rpl, p + ssl, strlen(p + ssl) + 1);
        }
    }
}

int binary_search(Item* ar, int size, const char *target) {
    int low = 0, high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int res = strcmp(ar[mid].name, target);
        if (res == 0) {
            return mid;
        }
        if (res < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

void macro(const char* filename_input, const char* filename_output) {
    char buffer[256];
    FILE* fp = fopen(filename_input, "r");
    if (!fp) {
        return;
    }
    FILE* fp2 = fopen(filename_output, "w");
    if (!fp2) {
        return;
    }

    int isMacro = 0;

    int macroItemsLength = 0;
    Item* macroItems = malloc(sizeof(Item));

    Item currentMacroItem;
    currentMacroItem.stringsLength = 0;
    currentMacroItem.strings = malloc(sizeof(char*));
    currentMacroItem.strings[0] = malloc(sizeof(char));
    currentMacroItem.name = malloc(sizeof(char));

    int currentArgumentInd = 0;
    int argumentsLength = 1;
    char** arguments = malloc(sizeof(char*));
    arguments[0] = malloc(sizeof(char));

    while (fgets(buffer, 256, fp) != NULL) {
        char* str = buffer;

        if (strstr(str, "MACRO") != NULL) {
            //заголовок макроопределения
            char* istr = strtok(str, " ,\n");
            currentMacroItem.name = realloc(currentMacroItem.name, sizeof(char)*(strlen(istr)+1));
            memcpy(currentMacroItem.name, istr, (strlen(istr)+1)*sizeof(char));

            currentArgumentInd = 0;
            isMacro = 1;
            istr = strtok(NULL, " ,\n");

            // получение и запись агрументов для последующей замены
            istr = strtok(NULL, " ,\n");
            while (istr != NULL) {
                arguments[currentArgumentInd] = realloc(arguments[currentArgumentInd], sizeof(char)*(strlen(istr)+1));
                memcpy(arguments[currentArgumentInd], istr, (strlen(istr)+1)* sizeof(char));
                currentArgumentInd++;
                if (currentArgumentInd >= argumentsLength) {
                    argumentsLength++;
                    arguments = realloc(arguments, sizeof(char*)*argumentsLength);
                    arguments[currentArgumentInd] = malloc(sizeof(char)*(strlen(istr)+1));
                }      
                istr = strtok(NULL, " ,\n");
            }
        } else if (strstr(str, "ENDM") != NULL) {
            // конец макроопределения
            macroItemsLength++;
            isMacro = 0;

            // запись тела макроопределения с заменёнными аргументами на ?0, ?1 и тд
            macroItems = realloc(macroItems, sizeof(Item)*macroItemsLength);
            macroItems[macroItemsLength-1].name = malloc(sizeof(char)*(strlen(currentMacroItem.name)+1));
            memcpy(macroItems[macroItemsLength-1].name, currentMacroItem.name,
             (strlen(currentMacroItem.name)+1)*sizeof(char));
            macroItems[macroItemsLength-1].stringsLength = currentMacroItem.stringsLength;
            macroItems[macroItemsLength-1].strings = realloc(macroItems[macroItemsLength-1].strings,
             sizeof(char*)*currentMacroItem.stringsLength);
            for (int i = 0; i < currentMacroItem.stringsLength; i++) {
                macroItems[macroItemsLength-1].strings[i] = malloc(sizeof(char)*(strlen(currentMacroItem.strings[i])+1));
                memcpy(macroItems[macroItemsLength-1].strings[i], currentMacroItem.strings[i],
                 sizeof(char)*(strlen(currentMacroItem.strings[i])+1));
            }
            // сортировка для будущего бинарного поиска
            qsort(macroItems, macroItemsLength, sizeof(Item), compareFunc);

            for (int i = 0; i < currentMacroItem.stringsLength; i++) {
                free(currentMacroItem.strings[i]);
            }
            currentMacroItem.stringsLength = 0;
            currentMacroItem.strings = realloc(currentMacroItem.strings, sizeof(char*));
        } else if (isMacro) {
            // тело макроопределения
            // замена аргументов на ?0, ?1 и тд 
            for (int i = 0; i < currentArgumentInd; i++) {
                char* findstr = strstr(str, arguments[i]);
                if (findstr != NULL) {
                    char num[15];
                    sprintf(num, "%d", i);
                    char buff[16];
                    strcpy(buff, "?");
                    strcat(buff, num);
                    replace(str, arguments[i], buff);
                }
            }
            currentMacroItem.stringsLength++;
            currentMacroItem.strings = realloc(currentMacroItem.strings, sizeof(char*)*currentMacroItem.stringsLength);
            currentMacroItem.strings[currentMacroItem.stringsLength-1] = malloc(sizeof(char)*(strlen(str)+1));
            memcpy(currentMacroItem.strings[currentMacroItem.stringsLength-1], str, (strlen(str)+1)*sizeof(char));
        } else {
            char* str2 = malloc(sizeof(char)*(strlen(str)+1));
            memcpy(str2, str, sizeof(char)*(strlen(str)+1));
            char* istr = strtok(str2, " ,\n");
            int bs_result = -1;
            if (istr != NULL) bs_result = binary_search(macroItems, macroItemsLength, istr);
            if (bs_result != -1) {
                // нашёл название макроопределения в программе
                char** args = malloc(sizeof(char*));
                int argsLength = 0;
                istr = strtok(NULL, " ,\n");
                while (istr != NULL) {
                    argsLength++;
                    args = realloc(args, sizeof(char*)*argsLength);
                    args[argsLength-1] = malloc(sizeof(char)*(strlen(istr)+1));
                    memcpy(args[argsLength-1], istr, (strlen(istr)+1)*sizeof(char));
                    istr = strtok(NULL, " ,\n");
                }
                // замена ?0, ?1 и тд на новые параметры
                Item item = macroItems[bs_result]; 
                for (int i = 0; i < item.stringsLength; i++) {
                    for (int j = 0; j < argsLength; j++) {
                        char num[15];
                        sprintf(num, "%d", j);
                        char buff[16];
                        strcpy(buff, "?");
                        strcat(buff, num);
                        replace(item.strings[i], buff, args[j]);
                    }
                    fprintf(fp2, "%s", item.strings[i]);
                }

                for (int i = 0; i < argsLength; i++) {
                    free(args[i]);
                }
                free(args);
            } else {
                // попалась обычная строка
                fprintf(fp2, "%s", buffer);
            }
            free(str2);
        }
    }

    for (int i = 0; i < currentMacroItem.stringsLength; i++) {
        free(currentMacroItem.strings[i]);
    }
    free(currentMacroItem.strings);
    free(currentMacroItem.name);
    
    for (int i = 0; i < argumentsLength; i++) {
        free(arguments[i]);
    }
    free(arguments);

    for (int i = 0; i < macroItemsLength; i++) {
        for (int j = 0; j < macroItems[i].stringsLength; j++) {
            free(macroItems[i].strings[j]);
        }
        free(macroItems[i].strings);
        free(macroItems[i].name);
    }
    free(macroItems);
    fclose(fp2);
    fclose(fp);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        return 1;
    }
    char* filename_input = argv[1];
    char* filename_output = argv[2];
    macro(filename_input, filename_output);
    return 0;
}