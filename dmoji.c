/* Compile:
 * $ gcc $(icu-config --ldflags --ldflags-icuio) <filename>
 * */

/* glibc */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <err.h>


/* Unicode */
#include <unicode/ustdio.h>
#include <unicode/utypes.h>
#include <unicode/uscript.h>
#include <unicode/uchar.h>
#include <unicode/uset.h>

/* 
 * ICU API
 * http://icu-project.org/apiref/icu4c/
 *
 *
 * References:
 * http://www.unicode.org/reports/tr51/#Emoji_Properties_and_Data_Files
 *
 * List of basic emoji sequences:
 *   + http://www.unicode.org/Public/emoji/latest/emoji-sequences.txt
 *
 * - Sets : http://icu-project.org/apiref/icu4c/uset_8h.html
 *
 * */

#ifndef DEBUG
#define DEBUG 0
#endif
#define DBG(...) do { if (DEBUG) dprintf(2, "DEBUG: " __VA_ARGS__); } while (0)

#define BUFF_SIZE 128

void describeSet(USet* set);
void describeUChar(UChar32 c);
void throwOneEmoji(int fd, UChar32 c);
int throwEmojisAt(int fd);

void print_ver();
void print_help();


int main(int argc, char** argv) {

    int c;
    while ((c = getopt (argc, argv, "hV")) != -1) {
        switch(c) {
            case 'V':
                print_ver();
                return 0;
            case 'h':
                print_help();
                return 0;
        }
    }

    int p_choices[2]; // The file descriptor set to pipe the list of choices. [0] is for reading, [1] is for writing
    int p_result[2]; // The file descriptor set to pipe the result
    pid_t pid;

    if ( pipe(p_choices) || pipe(p_result) ) {
        err(1, "unable to pipe one of the file descriptor set");
    }

    pid = fork();
    if ( pid == -1 ) {
        err(1, "fork dmenu");
    }

    if ( pid > 0 ) {
        // Parent: we write lines, wait for child and read
        //  - set p_choices as child's input
        //  - set p_result as child's output
        //  - waitfor

        char buff[BUFF_SIZE];
        int ret = 0;
        size_t size;

        close(p_choices[0]);
        close(p_result[1]);

        throwEmojisAt(p_choices[1]);
        close(p_choices[1]);
        wait(&ret);
        /* dmenu exit 0 on choice, 1 on no-choice (e.g.: escape) */
        if (WEXITSTATUS(ret) > 1 ) {
            warnx("dmenu seems to have returned an unusual exit: '%i'", WEXITSTATUS(ret));
        }

        /* Reminder: dmenu may return an arbitrary string, user typed. */
        size = read(p_result[0], buff, BUFF_SIZE);
        if ( size == 0 ) {
            // No choice selected
            DBG("No choice\n");
            return 0;
        }
        buff[size-1] = '\0';  // Cut at the '\n'

        DBG("choice is : '%s'\n", buff);

        // cut to the space
        char* p = strchr(buff, ' ');
        if (p) {
            *p = '\0';
        }
        DBG("sending to xsel: '%s'\n", buff);

        // Now we send this to xsel's stdin
        int p_clip[2];
        if (pipe(p_clip) ) {
            err(1, "Unable to pipe the clipboard file descriptor set");
        }

        pid = fork();
        if ( pid == -1 ) {
            err(1, "xsel fork");
        }
        if ( pid > 0 ) {
            // parent, similar as above:
            //  - write to p_clip
            //  - wait
            close(p_clip[0]);

            dprintf(p_clip[1], buff);
            close(p_clip[1]);
            wait(&ret);
            if (WEXITSTATUS(ret) > 0 ) {
                warnx("xsel seems to have returned an unusual exit: '%i'", WEXITSTATUS(ret));
            }
            return 0;

        } else {
            // Child
            //  - set p_clip as stdin
            char* const argv2[] = { "xsel", "--input", "--clipboard", NULL }; 

            close(p_clip[1]);
            if (dup2(p_clip[0], 0) == -1) {
                err(1, "dup p_clip to stdin");
            }
            close(p_clip[0]);
            execv("/usr/bin/xsel", argv2);

            // unreachable
            err(1, "execv xsel");
        }

    } else {

        // Child:
        //  - set p_choices as stdin
        //  - set p_result as stdout
        //  - execv

        char* const argv2[] = { "dmenu", "-i", NULL };

        close(p_choices[1]);
        close(p_result[0]);
        if (dup2(p_choices[0], 0) == -1) {
            err(1, "dup p_choices to stdin");
        }
        close(p_choices[0]);
        if (dup2(p_result[1], 1) == -1){
            err(1, "dup p_result to stdout");
        }
        close(p_result[1]);

        execv("/usr/bin/dmenu", argv2 );

        // unreachable
        err(1, "execv dmenu");
    }
}


/* 
 * Send a list of emojis to file descriptor provided
 * */
int throwEmojisAt(int fd) {

    int32_t count, total = 0;
    USet* emojis;
    UChar32 start = UCHAR_MIN_VALUE, end = UCHAR_MIN_VALUE;
    UChar u_buff[BUFF_SIZE];
    UErrorCode u_err = U_ZERO_ERROR;

    // TODO: ICU v.63 has u_getBinaryPropertySet(), to get straight away per-property sets.
    //       But this version is still too recent
    emojis = uset_openEmpty();
    uset_applyIntPropertyValue(emojis, UCHAR_EMOJI, TRUE, &u_err) ;
    uset_removeRange(emojis, UCHAR_MIN_VALUE, 0xff);   // some regular ASCII are caught into UCHAR_EMOJI for some reason. 

    if (u_err != U_ZERO_ERROR) {
        warnx("error while applying EMOJI property: %s", u_errorName(u_err));
        goto close_return;
    }
    
    DBG("emoji set after UCHAR_EMOJI restriction: %i items. Total size: %i\n", uset_getItemCount(emojis), uset_size(emojis));

    /* Dump everything to fd (separated by '\n')! */
    count = uset_getItemCount(emojis);
    for(int32_t i = 0; i < count; i++) {
        DBG("====== Item %i =======\n", i);
        UChar32 start = UCHAR_MIN_VALUE, end = UCHAR_MIN_VALUE;
        int32_t size = uset_getItem(emojis, i , &start, &end, u_buff, BUFF_SIZE, &u_err);
        if (u_err != U_ZERO_ERROR || size < 0) {
            warnx("error while getting first Item: %s", u_errorName(u_err));
            goto close_return;
        }
        
        // 2 solutions :
        //   - Item is a single unicode: the character is stored in u_buff & its length is count (>=2)
        //   - Item is a range: count is 0, start & end are defining the range
        if (size) {
            /* TODO : I'd like to remove that to avoid icuio library */
            u_printf("WARNING: got a character (length: %i): '%S'. Not supported at the moment\n", count, u_buff);
        } else {
            DBG("got a range (size %i). From 0x%x to 0x%x\n", (end - start + 1), start, end);
            for(UChar32 j = start; j <= end; j++) {
                total++;
                throwOneEmoji(fd, j);
            }
        }
    }

    DBG("total Emoji shown: %i\n", total);

close_return:
    DBG("Closing emoji set\n");
    uset_close(emojis);
    return 0;
}

void throwOneEmoji(int fd, UChar32 c) {
    int32_t count = 0;
    char buff[BUFF_SIZE];
    UErrorCode u_err = U_ZERO_ERROR;
    UBool err = FALSE;

    memset(buff, 0, BUFF_SIZE);
    U8_APPEND(buff, count, BUFF_SIZE, c, err);
    if (err) {
        warnx("could not append 0x%x to buff. Canceling this one", c);
        return;
    }
    strcat(buff, " ");
    count = strlen(buff);
    u_charName(c, U_UNICODE_CHAR_NAME, buff + count, BUFF_SIZE - count, &u_err);

    if (u_err != U_ZERO_ERROR) {
        warnx("while describing UChar32 0x%x: %s. Cancelling this one", c, u_errorName(u_err));
        return;
    }
    DBG("sending to fd %i: '%s'\n", fd, buff);
    dprintf(fd, "%s\n", buff);
}

void print_ver() {
    printf(TARGET " version: "  GIT_VERSION ", git " GIT_COMMIT "\n");
} 
void print_help() {
    print_ver();
    printf("\nCalls dmenu with a list all all base emojis available from the ICU library, then copies the selected emoji to clipboard\n");
}
