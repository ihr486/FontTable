#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <iconv.h>

#define MAX_ROW (256)
#define MAX_CHARS (65536)

typedef struct character_tag {
    uint16_t code;
    uint16_t w, h;
    uint16_t bitmap[16];
} character_t;

int ch_num = 0;
character_t *ch_list[MAX_CHARS];

static int pfxcmp(const char *a, const char *b)
{
    for(int i = 0; a[i] && b[i]; i++) {
        if(tolower(a[i]) > tolower(b[i]))
            return 1;
        if(tolower(a[i]) < tolower(b[i]))
            return -1;
    }
    return 0;
}

static void sort(void)
{
    int flag = 0;
    do {
        flag = 0;

        for(int i = 0; i < ch_num - 1; i++) {
            if(ch_list[i]->code > ch_list[i + 1]->code) {
                character_t *t = ch_list[i];
                ch_list[i] = ch_list[i + 1];
                ch_list[i + 1] = t;
                flag = 1;
            }
        }
    } while(flag);
}

static void print(void)
{
    printf("const int glyph_num = %d;\n"
           "const glyph_t glyphs[] = {\n", ch_num);
    for(int i = 0; i < ch_num; i++) {
        printf("\t{%u, %u,\n\t{", ch_list[i]->w, ch_list[i]->h);
        for(int y = 0; y < 15; y++) {
            printf("0x%X, ", ch_list[i]->bitmap[y]);
        }
        if(i < ch_num - 1) {
            printf("0x%X}\n\t},\n", ch_list[i]->bitmap[15]);
        } else {
            printf("0x%X}\n\t}\n", ch_list[i]->bitmap[15]);
        }
    }
    printf("};\n");

    printf("const uint16_t ucs_map[] = {\n");
    for(int i = 0; i < ch_num; i++) {
        printf("\t0x%X", ch_list[i]->code);
        if(i < ch_num - 1) {
            printf(",\n");
        } else {
            printf("\n");
        }
    }
    printf("};\n");
}

int main(int argc, const char *argv[])
{
    iconv_t ic = iconv_open("UCS-2", "EUC-JP");
    if((int)ic < 0) {
        fprintf(stderr, "Failed to open an iconv context.\n");
        return -1;
    }

    while(argc > 1) {
        FILE *fp = fopen(argv[1], "r");
        if(!fp) {
            fprintf(stderr, "Failed to open an input file: %s.\n", argv[1]);
            return -1;
        }

        character_t *c = NULL;
        char row[MAX_ROW];
        int y = -1;

        while(fgets(row, MAX_ROW, fp)) {
            if(!pfxcmp("STARTCHAR", row)) {
                c = (character_t *)calloc(sizeof(character_t), 1);
                y = -1;
            } else if(!pfxcmp("ENDCHAR", row)) {
                ch_list[ch_num++] = c;
            } else if(!pfxcmp("ENCODING", row)) {
                uint16_t code;
                sscanf(row, "%*s%hu", &code);
                uint8_t in_seq[8], out_seq[8];
                if(code < 0x80) {
                    in_seq[0] = code;
                    in_seq[1] = 0;
                } else {
                    in_seq[0] = (code >> 8) | 0x80;
                    in_seq[1] = code | 0x80;
                }
                in_seq[2] = 0;
                uint8_t *in_ptr = in_seq, *out_ptr = out_seq;
                size_t inbytes = 8, outbytes = 8;
                iconv(ic, &in_ptr, &inbytes, &out_ptr, &outbytes);
                c->code = (out_seq[1] << 8) | out_seq[0];
            } else if(!pfxcmp("BITMAP", row)) {
                y = 0;
            } else if(!pfxcmp("BBX", row)) {
                sscanf(row, "%*s%hu%hu", &(c->w), &(c->h));
            } else {
                if(y >= 0 && y < c->h) {
                    c->bitmap[y] = 0;
                    for(int i = 0; row[i]; i++) {
                        if(row[i] == '.') {
                            c->bitmap[y] <<= 1;
                        } else if(row[i] == '@') {
                            c->bitmap[y] = 1 | (c->bitmap[y] << 1);
                        } else if('0' <= row[i] && row[i] <= '9') {
                            c->bitmap[y] = (row[i] - '0') | (c->bitmap[y] << 4);
                        } else if('a' <= row[i] && row[i] <= 'f') {
                            c->bitmap[y] = (row[i] - 'a' + 10) | (c->bitmap[y] << 4);
                        } else if('A' <= row[i] && row[i] <= 'F') {
                            c->bitmap[y] = (row[i] - 'A' + 10) | (c->bitmap[y] << 4);
                        }
                    }
                    y++;
                }
            }
        }

        fclose(fp);

        argv++;
        argc--;
    }

    sort();

    print();

    iconv_close(ic);

    return 0;
}
