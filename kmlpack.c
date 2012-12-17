#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <db.h>

static int put_file ( char * name, char * key, size_t size, DB * db ) 
{
    FILE * file;
    DBT dbkey, dbdata;
    int retval = 0;

    memset( &dbkey, 0, sizeof( dbkey ) );
    memset( &dbdata, 0, sizeof( dbdata ) );
    dbkey.data = key;
    dbkey.size = strlen( key );
    dbdata.size = size;

    dbdata.data = malloc( size );
    if( dbdata.data == NULL ) {
        fprintf( stderr, "Can't allocate memory\n" );
        exit( 1 );
    }

    file = fopen( name, "rb" );
    if( file ) {
        if( ( size == 0 ) || ( fread( dbdata.data, size, 1, file ) == 1 ) ) {
            if( db->put( db, NULL, &dbkey, &dbdata, 0 ) != 0 ) {
                fprintf( stderr, "Can't put data into database for file %s\n", 
                    name );
                retval = 1;
            }
        } else {
            fprintf( stderr, "Can't read from file %s\n", name );
            retval = 1;
        }
        fclose( file );
    } else {
        fprintf( stderr, "Can't open file %s\n", name );
        retval = 1;
    }

    free( dbdata.data );
    return retval;
}

static int list_files ( char * name, size_t base_len, DB * db )
{
    DIR * handle;
    struct dirent * entry;
    struct stat info;
    size_t len = strlen( name );
    int retval = 0;
    long lastidx, nextidx;

    handle = opendir( name );
    if( handle ) {
        name[len] = '/';
        name[len+1] = '\0';
        lastidx = telldir( handle );
        while( ( entry = readdir( handle ) ) != NULL ) {
            nextidx = telldir( handle );
            if ( nextidx == lastidx ) {
                fprintf(stderr, "Directory corruption at %s\n", name);
                break;
            }
            lastidx = nextidx;
            if( strcmp( entry->d_name, "." ) != 0 && 
                    strcmp( entry->d_name, ".." ) != 0) {
                if( len + strlen( entry->d_name ) + 2 > PATH_MAX ) {
                    fprintf( stderr, "Skipping %s%s - filename is too long\n",
                        name, entry->d_name );
                    retval = 1;
                    continue;
                }
                strcpy( name + len + 1, entry->d_name );
                retval = stat( name, &info );
                if (retval == 0) {
                    if( S_ISREG( info.st_mode ) ) {
                        retval |= put_file( 
                             name, name + base_len, info.st_size, db );
                    } else if( S_ISDIR( info.st_mode ) ) {
                         retval |= list_files( name, base_len, db );
                    } else {
                         fprintf( stderr, "Strange file %s\n", name );
                    }
               } else {
                     fprintf( stderr,"Failed to stat file %s\n", name );
                     perror( "stat" );
               }
            }
        }
        closedir( handle );
        name[len] = '\0';
    } else {
        fprintf( stderr, "Can't open dir %s\n", name );
        retval = 1;
    }

    return retval;
}

int main ( int argc, char ** argv )
{
    char base_dir[PATH_MAX];
    char * db_file;
    int retval = 0;
    DB * db;

    if( argc == 3 ) {
        if( strlen( argv[1] ) + 1 > PATH_MAX ) {
            fprintf( stderr, "Can't process %s - path is too long\n", argv[1] );
            return 1;
        }
        strcpy( base_dir, argv[1] );
        db_file = argv[2];
    } else {
        fprintf( stderr, "Usage: %s <dir> <db_file>\n", argv[0] );
        return 1;
    } 

    if( db_create( &db, NULL, 0 ) != 0 ) {
        fprintf( stderr, "Can't create db\n" );
        return 1;
    }

    if( db->open( db, NULL, db_file, NULL, DB_HASH, DB_CREATE, 0644 ) != 0 ) {
        fprintf( stderr, "Can't open database %s\n", db_file );
        db->close( db, 0 );
        return 1;
    }

    retval = list_files( base_dir, strlen( base_dir ) + 1, db );
    db->close( db, 0 );

    return retval;
}
