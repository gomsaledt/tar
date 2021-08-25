# Tar implementation in C

## Description

- **Tar** - saves many files together into a single archive file known as tarfile.

- Can compress by using compression programs like gzip, bzip2, lzip, etc.

## Synopsis

 `tar  {a | c | t |u | x | z} [args..]`

## Options

| option | Description|
---|---
| a | append to archive |
| c | create a new archive |
| t | ist the files in the archive |
| u | decompress archive |
| x | extract from archive |
| z | compress archive |
| help | to get help |

## Algorithm

**Huffman Coding.** (for compression)

## Used Function and their Description

### int Main()

This is the main function where the input from user is processed.

### int write_tar()

This function write filedata to a tar file. If archive contains data, the new data will be appended to the back of the file.

### int read_tar()

This function read a tar file and  form the linked list of members.

### int extract_tar()

This function extract the tar file to its original files and folders.

### int print_metadata()

This function prints the data of files (e.g. name, uid, gid, size, etc)
to the output screen.

### void compress_file()

This function compress the tar file. 

### void  uncompress_file()

This function decompress back compressed file to tar file.
