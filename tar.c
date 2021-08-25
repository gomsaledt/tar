#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<dirent.h>
#include<time.h>
#include"tar.h"



///////////////////////////// utility functions ////////////////////////////////////

// init a tar
void initTar(tar *t){
    *t = NULL;
    return;
}


// destroy tar
int destroy_tar(tar *t){
    if(*t == NULL){
        return 0;
    }
    member *p = *t, *q = NULL;;
    while(p){
        q = p;
        p = p->next;
        free(q);
    }
    return 0;
}

// convert oct to unsigned int
unsigned int oct2uint(char *oct, unsigned int size){
    unsigned int out = 0;
    int i = 0;
    while ((i < size) && oct[i]){
        out = (out << 3) | (unsigned int) (oct[i++] - '0');
    }
    return out;
}


// set member data accordingly
int format(member *m, char *filename){
    // status of file
    struct stat st;
    if(stat(filename, &st)){
        printf("Cannot stat %s\n", filename);
        return -1;
    }

    // remove relative path
    int move = 0;
    if(!strncmp(filename, "/", 1))
        move = 1;
    else if(!strncmp(filename, "./", 2))
        move = 2;
    else if(!strncmp(filename, "../", 3))
        move = 3;

    // start putting new data
    memset(m, 0, sizeof(member));
    strncpy(m->name, filename + move, 100);
    snprintf(m->mode, sizeof(m->mode), "%07o", st.st_mode & 0777);
    snprintf(m->uid, sizeof(m->uid), "%07o", st.st_uid);
    snprintf(m->gid, sizeof(m->gid), "%07o", st.st_gid);
    snprintf(m->size, sizeof(m->size), "%011o", (int) st.st_size);
    snprintf(m->mtime, sizeof(m->mtime), "%011o", (int) st.st_mtime);

    // check for type
    if((st.st_mode & S_IFMT) == S_IFDIR){
        memset(m->size, '0', 11);
        m->typeflag = DIRTYPE;

        // save parent dir name
        int len = (int)strlen(m->name);
        char *parent = (char *)calloc(len + 1, sizeof(char));
        strncpy(parent, m->name, len);

        // add a '/' character to end
        if(len < 99 && (m->name[len - 1] != '/')){
            m->name[len] = '/';
            m->name[len + 1] = '\0';
            }
    }
    else
        m->typeflag = FILETYPE;
    strncpy(m->start, "0", 8);
    strncpy(m->end, "0", 8);

    return 0;
}



// write file data to tar file
unsigned int write_to_tar(FILE *f, char *filename){
    FILE *fin = fopen(filename, "r");
    unsigned int offset = 0;
    char c;
    while((c = fgetc(fin)) != EOF){
        fputc(c, f);
        offset++;
    }
    fclose(fin);
    return offset;
}


int recursive_mkdir(char *dir, unsigned int mode){
    int rc = 0;
    size_t len = strlen(dir);

    if (!len){
        return 0;
    }

    char *path = (char*)calloc(len + 1, sizeof(char));
    strncpy(path, dir, len);

    // remove last '/'
    if (path[len - 1] ==  '/'){
       path[len - 1] = 0;
    }

    // all subsequent directories do not exist
    for(char *p = path + 1; *p; p++){
        if (*p == '/'){
            *p = '\0';

            if ((rc = mkdir(path))){
                printf("Could not create directory %s", path);
                return -1;
            }

            *p = '/';
        }
    }

    if (mkdir(path) < 0){
        printf("Could not create directory %s", path);
        return -1;
    }

    free(path);
    return 0;
}


// write back to file
int write_from_tar(member m, FILE *f, char *filename, unsigned int *offset){
    if(m.typeflag == FILETYPE){
        FILE *fin = fopen(filename, "w");
        if(!fin){
            printf("Cannot write to file: %s\n", filename);
            return -1;
        }
        fseek(f, *offset, SEEK_SET);

        unsigned int current_pos = 0;
        char c;
        while(current_pos < oct2uint(m.size, 11)){
            c = fgetc(f);
            fputc(c, fin);
            current_pos++;
        }

        setmode(fileno(fin), oct2uint(m.mode, 7));

        fclose(fin);
        *offset += oct2uint(m.size, 11);
    }
    else{
        if(recursive_mkdir(m.name, oct2uint(m.mode, 7) & 0777) < 0){
            printf("Unable to create directory %s", m.name);
            return -1;
        }
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////// core functions //////////////////////////////////////

// read a tar file
// t should be address to null pointer
int read_tar(tar *t, char *filename){
    FILE *f = fopen(filename, "r");
    if(!f){
        printf("Bad file descriptor\n");
        return -1;
    }
    struct stat st;
    stat(filename, &st);
    unsigned int offset = 0;
    member *p, *q = NULL;
    while(offset + BLOCKSIZE < (int)st.st_size){
        p = (member *)malloc(sizeof(member));
        fscanf(f, "%100s", p->name);
        fscanf(f, "%8s", p->mode);
        fscanf(f, "%8s", p->uid);
        fscanf(f, "%8s", p->gid);
        fscanf(f, "%12s", p->size);
        fscanf(f, "%12s", p->mtime);
        fscanf(f, "%c", &(p->typeflag));
        fscanf(f, "%8s", p->start);
        fscanf(f, "%8s", p->end);
        p->next = NULL;
        offset += BLOCKSIZE + oct2uint(p->size, 11);
        fseek(f, offset, SEEK_SET);
        if(*t == NULL)
            *t = p;
        else
            q->next = p;
        q = p;
    }
    fclose(f);
    return 0;
}




// write to a tar file
// if archive contains data, the new data will be appended to the back of the file
int write_tar(tar *t, FILE *f, int filecount, char *files[]){
    if(!f){
        printf("Bad file descripter\n");
        return -1;
    }

    // file descriptor offset
    int offset = 0;

    // if there is old data
    member *p = *t;
    if(p){
        // skip to last member
        while(p && p->next)
            p = p->next;

        // get offset past final member
        unsigned int jump = BLOCKSIZE + oct2uint(p->size, 11);

        // move file descriptor
        offset = oct2uint(p->start, 7) + jump;
        if(fseek(f, offset, SEEK_SET) == (off_t) (-1)){
            printf("Unable to seek file\n");
            return -1;
        }
        p = p->next;
    }

    // add new data
    for(int i = 0; i < filecount; i++){
        p = (member *)malloc(sizeof(member));

        format(p, files[i]);
        offset += BLOCKSIZE;
        snprintf(p->start, sizeof(p->start), "%07o", (unsigned int)ftell(f) + BLOCKSIZE);
        snprintf(p->end, sizeof(p->end), "%07o", (unsigned int)ftell(f) + BLOCKSIZE + oct2uint(p->size, 7));

        printf("Writing %s\n", p->name);

        // write metadata to tar file
        fwrite(p, BLOCKSIZE, 1, f);

        if(p->typeflag == FILETYPE){
            // writing content of file
            offset += write_to_tar(f, files[i]);
        }
        else{
            // save parent directory name
            const size_t len = strlen(p->name);
            char *parent = (char *)calloc(len + 1, sizeof(char));
            strncpy(parent, p->name, len);

            // go through directory
            DIR *d = opendir(parent);
            if(!d){
                printf("Cannot open directory: %s\n", parent);
                return -1;
            }

            struct dirent *dir;
            while((dir = readdir(d))){
                // if not special directories . and ..
                const size_t sublen = strlen(dir->d_name);
                if(strncmp(dir->d_name, ".", sublen) && strncmp(dir->d_name, "..", sublen)){
                    char *path = (char *)calloc(len + sublen + 2, sizeof(char));
                    sprintf(path, "%s/%s", parent, dir->d_name);

                    // recursively write each subdirectory
                    offset += write_tar(&(p->next), f, 1, (char **) &path);

                    // go to end of new data
                    while (p->next){
                        p = p->next;
                    }

                    free(path);
                }
            }
            closedir(d);

            free(parent);

        }
             p = p->next;
    }

    return offset;
}


// extract tar file to original files
int extract_tar(tar *t, FILE *f){
    if(!f){
        printf("Bad file descriptor\n");
        return -1;
    }

    // if tar is empty
    if(*t == NULL){
        printf("Empty tar file\n");
        return -1;
    }

    member *p = *t;
    unsigned int offset = 0;
    while(p){
        offset += BLOCKSIZE;
        fseek(f, offset, SEEK_SET);

        printf("Extracting %s\n", p->name);
        write_from_tar(*p, f, p->name, &offset);
        p = p->next;
    }

    return 0;
}

// prints the metadata of members of tar files
int print_metadata(tar t){
    if(!t){
        printf("Empty tar\n");
        return -1;
    }
    member *p = t;
    printf("mode\tuid\tgid\tsize\tmodified\tname\n");
    while(p){
        time_t mtime = oct2uint(p->mtime, 12);
        char mtime_str[32];
        strftime(mtime_str, sizeof(mtime_str), "%c", localtime(&mtime));
        if(p->typeflag == DIRTYPE)
            printf("d");
        else
            printf("-");
        printf("%d\t", oct2uint(p->mode, 8));
        printf("%d\t", oct2uint(p->uid, 8));
        printf("%d\t", oct2uint(p->gid, 8));
        printf("%d Kb\t", oct2uint(p->size, 12));
        printf("%s\t", mtime_str);
        printf(p->name);
        printf("\n");

        p = p->next;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////
