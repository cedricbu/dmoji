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
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>


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
#define COMMENT_CHAR ' '


void describeSet(USet* set);
void describeUChar(UChar32 c);
void throwOneEmoji(int fd, UChar32 c);
int throwEmojisAt(int fd);
int additional_path(int fd, const char* file);
int additional_file(int fd_out, const char* path);
int clean_output(const char* sep, char* str);
pid_t popen2(const char* cmd, char* const argv[], int* in, int* out);

void print_ver();
void print_help();

const char* separator = " ;";

char* dmenu_argv[] = { "dmenu", "-i", NULL };

char* rofi_argv[] = { "rofi", "-dmenu", "-i", NULL };

char* const xsel_argv[] = { "xsel", "--input", "--clipboard", NULL }; 

int main(int argc, char** argv) {
    int c, opt_a=0;
    char append[BUFF_SIZE];
    char** menu_argv = dmenu_argv;

    while ((c = getopt (argc, argv, "hVa:r")) != -1) {
        switch(c) {
            case 'V':
                print_ver();
                return 0;
            case 'h':
                print_help();
                return 0;
            case 'a':
                opt_a = 1;
                strncpy(append, optarg, BUFF_SIZE);
                break;
            case 'r':
                DBG("Using Rofi menu\n");
                menu_argv = rofi_argv;
                break;
        }
    }

    DBG("Launching menu\n");
    int child_in, child_out;
    int ret;
    char buff[BUFF_SIZE];
    size_t size;
    pid_t pid;

    pid = popen2( menu_argv[0], menu_argv, &child_in, &child_out );
    if ( pid < 0 ) {
        errx(1, "Unable to popen2 %s", menu_argv[0]);
    }

    throwEmojisAt(child_in);
    if (opt_a) {
         additional_path(child_in, append);
    }
    close(child_in);
    wait(&ret);

    /* dmenu exit 0 on choice, 1 on no-choice (e.g.: escape) */
    if (WEXITSTATUS(ret) > 1 ) {
        warnx("dmenu seems to have returned an unusual exit: '%i'", WEXITSTATUS(ret));
    }

    /* Reminder: dmenu may return an arbitrary string, user typed. */
    size = read(child_out, buff, BUFF_SIZE -1);
    close(child_out);
    if ( size < 0) {
        err(1, "Could not read output from dmenu");
    }

    if ( size == 0 || WEXITSTATUS(ret) == 1 ) {
        DBG("No choice\n"); // No choice selected
        return 0;
    }
    if ( clean_output(separator, buff) ) {
        DBG("Cut '%s' from '%s'\n", separator, buff);
    } else {
        DBG("'%s' was not found in returned '%s'\n", separator, buff);
    }

    DBG("sending to xsel: '%s'\n", buff);

    pid = popen2( xsel_argv[0], xsel_argv, &child_in, &child_out);
    close(child_out);
    if ( pid < 0 ) {
        errx(1, "Unable to popen2 %s", xsel_argv[0]);
    }
    dprintf(child_in, buff);
    close(child_in);
    wait(&ret);
    if (WEXITSTATUS(ret) > 0 ) {
        warnx("xsel seems to have returned an unusual exit: '%i'", WEXITSTATUS(ret));
    }
    return 0;
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

    if ( !U_SUCCESS(u_err) ) {
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
        if ( !U_SUCCESS(u_err) || size < 0) {
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

/* 
 * write `c` and its name (space separated) to file descriptor fd)
 *
 * */
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
    strcat(buff, separator);
    count = strlen(buff);
    u_charName(c, U_UNICODE_CHAR_NAME, buff + count, BUFF_SIZE - count, &u_err);

    if ( !U_SUCCESS(u_err) ) {
        warnx("while describing UChar32 0x%x: %s. Cancelling this one", c, u_errorName(u_err));
        return;
    }
    DBG("sending to fd %i: '%s'\n", fd, buff);
    dprintf(fd, "%s\n", buff);
}

/*
 * Remove possible newline
 * Search for `sep` in `str`
 * Replace the first character of `sep` in `str` with '\0'
 * Return: 1 if `sep` was found, 0 otherwise
 * */
int clean_output(const char* sep, char* str) {
    char* p;
    if ( p = strchr(str, '\n') ) {
        *p = '\0';  // Remove the new line if there is one
    }

    p = str;
    while ( p && *p != '\0' ) {
        p = strchr(p, *sep);
        if ( p == NULL ) {
            // no more possible match
            return 0;
        }

        for ( int i = 1; ; i++) {
            if ( *(sep + i*sizeof(char)) == '\0') {
                // iterated the whole `sep` with no exit: we found it!
                *p = '\0';
                return 1;
            }
            if ( *(p + i*sizeof(char)) == '\0' ) { // test for end of string
                return 0;
            }

            if ( *(p + i*sizeof(char)) != *(sep + i*sizeof(char)) ) {
                break;  // not a match
            }
        }
        
        p += sizeof(char); // next position and start again
    }
    return 0;
}

/*
 * Read list of additional stuff
 *
 * */
int additional_path(int fd, const char* path) {
    int count = 0;
    struct stat st;

    DBG("Additional path from entry'%s'\n", path);

    if ( stat(path, &st) ) {
        warn("Unable to stat %s. Make sure the path exists and is accessible. Ignoring.", path);
        return 0;
    }
    if (S_ISDIR(st.st_mode)) {
        /* check all regular files from this directory */
        DIR* dir = opendir(path);
        struct dirent* dirt;
        char final[BUFF_SIZE];

        DBG("'%s' is a dir. Reading dir\n", path);

        strcpy(final, path);
        strncat(final, "/", BUFF_SIZE - strlen(final) - 1);

        if ( dir == NULL ) {
            warn("Unable to open directory %s. Ignoring.", path);
            return 0;
        }
        while ( dirt = readdir(dir) ) {
            /* Only use the file if it is regular or link */
            if ( dirt->d_type == DT_REG || dirt->d_type == DT_LNK ) {
                strncpy(final + (strlen(path)+1)*sizeof(char), dirt->d_name, BUFF_SIZE - strlen(path) - 2);
                DBG("Adding entry %s.\n", final);
                count += additional_file(fd, final);
            } else {
                DBG("Ignoring entry %s, not a regular or symlink\n", final);
            }
        }
        closedir(dir);
        

    } else if (S_ISREG(st.st_mode)) {
        count = additional_file(fd, path);
    } else {
        warnx("'%' Does not appear to be a supported file. It must be either regular or directory. Ignoring.", path);
        return 0;
    }

    return count;
}

int additional_file(int fd_out, const char* path) {
    FILE* f;
    char buff[BUFF_SIZE];
    size_t len;
    int count;
    struct stat st;

    if ( stat(path, &st) || !S_ISREG(st.st_mode)) {
        warn("Unable to stat %s, or is not a regular file. Make sure the file exist and is a regular file. Ignoring", path);
        return 0;
    }
    f = fopen(path, "r");
    if ( !f ) {
        warn("Unable to open %s for reading. Ignoring", path);
        return 0;
    }

    while ( fgets(buff, BUFF_SIZE - 1, f)) {  // Read a full line
        // Lines starting with a space (' ') are considered as comments
        // TODO: Remove the EOL from this
        if ( buff[0] == COMMENT_CHAR) {
            char* nl = index(buff, '\n');
            if (nl != NULL) {
                *nl = '\0';
            }
            DBG("Ignoring comment '%s'\n", buff);
            continue;
        }
        // Ensure there is a newline
        len = strlen(buff);
        if ( buff[len] != '\n' ) {
            buff[len] = '\n';
            buff[len + 1] = '\0';
        }
        if ( len > 1 ) {
            DBG("Adding an entry of %i size\n", len);
            count++;
            dprintf(fd_out, buff);
        } else {
            DBG("Ignoring empty line\n");
        }
    }

    fclose(f);
    return count;
}

/* 
 * Exec `cmd` with args `argv[]`. 
 * fill up `in` and `out` piping to cmd
 * returns cmd's PID (-1 on error. A warning with errno will be printed)
 *
 * */
pid_t popen2(const char* cmd,  char* const argv[], int* in, int* out) {
    DBG("Popen2: %s\n", cmd);

    int p_in[2];
    int p_out[2];
    pid_t pid;

    if ( pipe(p_in) || pipe(p_out) ) {
        warn("Unable to pipe one of the pipes for command %s", cmd);
        return -1;
    }

    pid = fork();
    if ( pid < 0 ){
        warn("Unable to fork to command %s", cmd);
        return pid;
    }

    if ( pid > 0) {
        // Parent
        close(p_in[0]); // close the reader's input
        close(p_out[1]); // close the writer's output
        *in = p_in[1];
        *out = p_out[0];
        return pid;
    } else {
        // Child
        close(p_in[1]);
        close(p_out[0]);
        if (dup2(p_in[0], 0) == -1 || dup2(p_out[1], 1) == -1) {
            // post fork: we can exit rather than return
            err(1, "command %s: unable to dup one of stdin/stdout", cmd);
        }

        execvp(cmd, argv );

        // unreachable
        err(1, "execvp %s failed", cmd);
    }
}

void print_ver() {
    printf(TARGET " version: "  GIT_VERSION ", git " GIT_COMMIT "\n");
} 
void print_help() {
    print_ver();
    printf("\nCalls dmenu or rofi with a list of all base emojis available from the ICU library, then copies the selected emoji to clipboard\n");
    printf("Options:\n");
    printf(" -r\t\tUse Rofi instead of dmenu (Rofi will be started in dmenu mode)\n");
    printf(" -a <path>\tFile/directory containing additional data (e.g.: ASCII arts, or more complex Unicode sequences)\n");
    printf("\t\tAnything after the separator will be discarded before being sent to the clipboard\n");
    printf("\n\nAdditional data file:\n");
    printf("* Each line represents a new entry: the part to be copied, optionally followed by a separator ('%s') and a description to help the search\n", separator);
    printf("* Lines starting with a space (i.e.: ' ') are ignored as comments\n");
    printf("\n");
}
