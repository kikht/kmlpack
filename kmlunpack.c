#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <db.h>

static int mkpath( char * path ) {
    int status = 0;
    char * pos = path;

    if( *pos == '/' ) ++pos;
    while( status == 0 && ( pos = strchr( pos, '/' ) ) != NULL ) {
        *pos = '\0';
        status = mkdir( path, 0755 );
        if( status  && errno == EEXIST ) status = 0;
        *pos++ = '/';
    }
    return status;
}

int main ( int argc, char ** argv )
{
    char path[PATH_MAX];
    char * db_file;
    int retval = 0;
    size_t base_len;
    DB * db;
    DBC * cursor;
    DBT key;
    DBT value;
    FILE * file;

    if( argc != 3 ) {
        fprintf( stderr, "Usage: %s <db_file> <dir>\n", argv[0] );
        return 1;
    } 

    db_file = argv[1];
    if( strlen( argv[2] ) + 2 > PATH_MAX ) {
        fprintf( stderr, "Can't unpack to %s - path is too long\n", argv[2] );
        return 1;
    }
    strcpy( path, argv[2] );
    base_len = strlen( path );
    if( path[base_len-1] != '/' ) {
        path[base_len++] = '/';
        path[base_len] = '\0';
    }

    if( db_create( &db, NULL, 0 ) != 0 ) {
        fprintf( stderr, "Can't create db\n" );
        return 1;
    }

    memset( &key, 0, sizeof( key ) );
    memset( &value, 0, sizeof( value ) );
    key.flags = DB_DBT_REALLOC;            
    value.flags = DB_DBT_REALLOC;

    if( db->open( db, NULL, db_file, NULL, DB_UNKNOWN, DB_RDONLY, 0 ) == 0 ) {
        if ( db->cursor( db, NULL, &cursor, 0) == 0 ) {
            while( cursor->get( cursor, &key, &value, DB_NEXT ) == 0 ) {
                path[base_len] = '\0';
                if( base_len + key.size + 1 > PATH_MAX ) {
                    fprintf( stderr, "Skipping file, name is too long\n" );
                    retval = 1;
                    continue;
                }
                memcpy( path + base_len, key.data, key.size );
                path[base_len + key.size] = '\0';
                if( mkpath( path ) ) {
                    fprintf( stderr, "Can't create path for %s\n", path );
                    retval = 1;
                    continue;
                }
                file = fopen( path, "wb" );
                if( !file ) {
                    fprintf( stderr, "Can't open file %s\n", path );
                    retval = 1;
                    continue;
                }
                if( fwrite( value.data, value.size, 1, file ) != 1 ) {
                    fprintf( stderr, "Can't write to %s\n", path );
                    retval = 1;
                }
                fclose( file );
            }
            cursor->close( cursor );
        } else {
            fprintf( stderr, "Can't create database cursor\n" );
            retval = 1;
        }
    } else {
        fprintf( stderr, "Can't open database %s\n", db_file );
        retval = 1;
    }

    db->close( db, 0 );
    if( key.data != NULL ) {
        free( key.data );
        key.data = NULL;
    }
    if( value.data != NULL ) {
        free( value.data );
        value.data = NULL;
    }

    return retval;
}
