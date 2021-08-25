#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include"tar.h"
#include"huffman.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))


//int main(int argc, ){
//    tar t;
//    initTar(&t);
//    FILE *ft = fopen("a.tar", "a+");
//    //char *files[] = {"t1.txt", "t3.txt"};
//    //unsigned int n = write_tar(&t, ft, 2, files);
//    read_tar(&t, "a.tar");
//    extract_tar(&t, ft);
//    print_metadata(t);
//    fclose(ft);
//    return 0;
//}

int main(int argc, char *argv[]){
    if(((argc == 2) && (strncmp(argv[1], "help", MAX(strlen(argv[1]), 4)))) || (argc < 3)){
        printf("Usage: %s option(s) tarfile [sources]\n", argv[0]);
        printf("Usage: %s help\n", argv[0]);
        return -1;
    }

    if(argc == 2){
        printf("Usage: %s option(s) tarfile [sources]\nUsage: %s help\n\nOptions:\n\tc - create a new archive\n\ta - append to archive\n\tt - list the files in the archive\n\tx - extract from archive\n\n" , argv[0], argv[0]);
        printf("Other options:\n\tz - compress archive\n\tu - decompress archive\n");
      return 0;
    }
    argc -= 3;

    int rc = 0;
    char c = 0,             // create
         a = 0,             // append
         t = 0,             // list
         x = 0,             // extract
         z = 0,             // compress
         u = 0;             // decompress

    // parse options
    for(int i = 0; argv[1][i]; i++){
        switch (argv[1][i]){
            case 'c': c = 1; break;
            case 't': t = 1; break;
            case 'x': x = 1; break;
            case 'a': a = 1; break;
            case '-': break;
            case 'z': z = 1; break;
            case 'u': u = 1; break;
            default:
                printf("Error: Bad option: %c\n", argv[1][i]);
                printf("Do '%s help' for help\n", argv[0]);
                return 0;
                break;
        }
    }

    // make sure only one of these options was selected
    char used = c + t + x + a + z + u;
    if(used > 1){
        printf("Error: Cannot have so all of these flags at once\n");
        return -1;
    }
    else if(used < 1){
        printf("Error: Need one of 'ctxzu' options set\n");
        return -1;
    }

    char *filename = argv[2];
    char **files = (char **) &argv[3];

    // //////////////////////////////////////////

    tar tar;
    initTar(&tar);

    if(c){             // create new file
        FILE *f = fopen(filename, "w+");
        if(!f){
            printf("Error: Unable to open file %s\n", filename);
            return -1;
        }

        if(write_tar(&tar, f, argc, files) < 0){
            rc = -1;
        }
        fclose(f);
    }
    else{
        // open existing file
        FILE *f = fopen(filename, "a+");

        if(!f){
            printf("Error: Unable to open file %s\n", filename);
            return -1;
        }

        // read in data
        if(read_tar(&tar, filename) < 0){
            destroy_tar(&tar);
            return -1;
        }

        // perform operation
        if((a && (write_tar(&tar, f, argc, files) < 0))       ||   // append
            (t && (print_metadata(tar) < 0))                  ||   // list entries
            (x && (extract_tar(&tar, f) < 0))                      // extract entries
            ){
            printf("Exiting with error due to previous error\n");
            rc = -1;
        }
        fclose(f);
    }

    destroy_tar(&tar);

    if(z){
        char newname[103];
        sprintf(newname, "%s%s",  filename, ".gz");
        compress_file(filename, newname);
        rc = 0;
    }
    else if(u){
        char newname[100];
        snprintf(newname, sizeof(filename) - 3, "%s",  filename);
        uncompress_file(filename, newname);
        rc = 0;
    }
    return rc;
}
