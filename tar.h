
// type of member values
#define DIRTYPE '0'
#define FILETYPE '1'

#define BLOCKSIZE 168


// tar members
typedef struct member{        /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char typeflag;                /* 148 */
    char start[8];                /* 149 */
    char end[8];                  /* 153 */
    struct member *next;          /* 157 */
                                  /* 161 */
}member;

typedef member* tar;

// core functions //////////////////////////////////////////////////////////////

// init a tar
void initTar(tar *t);

// read a tar file
// t should be address to null pointer
int read_tar(tar *t, char *filename);

// write to a tar file
// if archive contains data, the new data will be appended to the back of the file
int write_tar(tar *t, FILE *f, int filecount, char *files[]);

// extract tar file to original files
int extract_tar(tar *t, FILE *f);

// prints the metadata of members of tar files
int print_metadata(tar t);


// destroy tar
int destroy_tar(tar *t);
// /////////////////////////////////////////////////////////////////////////////


