//
// Created by paul on 11/8/20.
//

#include "mungeFCC.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define kHeaderFilename "fccdata.h"
const char * kHeaderPrefix = "struct FCCStationData[] = \n{\n";
const char * kHeaderSuffix = "};\n";

#define kHashFilename "fccdata.hash"
const char * kHashPrefix = "\n"
    "prefix = \"US\"\n"
    "mappings : {\n"
    "    ignoreCase = true\n"
    "    Separator = \" :|()[]-\"\n"
    "}\n\n"
    "keywords = [\n";

const char * kHashSuffix = "]\n";

typedef struct sStation {
    struct sStation * next;
    const char *      callsign;
    const char *      city;
    const char *      state;
    int               logicalChan;
} tStation;

int lenTrimmed( char * field, unsigned int length )
{
    int result = 0;
    char * p = field;

    if (length > 0 )
    {
        p += length - 1;
        while (  p > field && isspace( *p ) ) { --p; }
        result = (p - field) + 1;
    }
    return result;
}

void toTitleCase( char * title )
{
    int first = 1;
    char * p = title;
    while ( *p != '\0' )
    {
        if ( isspace(*p) || ispunct( *p ) )
        {
            first = 1;
        } else if (first) {
            *p = toupper(*p);
            first = 0;
        } else {
            *p = tolower(*p);
        }
        p++;
    }
}

int processLine( const char * line, tStation * station )
{
    int result = 0;
    const char * p = line;

    int    fieldNum;
    char   field[64];
    char * fp;
    int    fl;
    char * r;

    fieldNum = 1;
    while ( *p != '\0' )
    {
        fp = field;
        fl = sizeof( field );

        r  = NULL;
        while ( *p != '|' && *p != '\0' && fl > 0 )
        {
            *fp++ = *p;
            --fl;
            if ( isgraph(*p) )
            {
                r = fp;
            }
            ++p;
        }

        if ( r != NULL ) { *r = '\0'; }

        switch ( fieldNum )
        {
        case 1: // station callsign
            {
                char * dash = strchr( field, '-' );
                if ( dash != NULL) { *dash = '\0'; }

                station->callsign = strdup( field );
            }
            break;

        case 2: // city
            toTitleCase( field );
            station->city = strdup( field );
            break;

        case 3: // state
            station->state = strdup( field );
            break;

        case 4: // logical channel
            station->logicalChan = atoi(field);
            break;

        default:
            break;
        }

        if ( *p != '\0' ) { ++p; }
        ++fieldNum;
    }

    return result;
}

int main( int argc, const char *argv[] )
{
    int  result = 0;
    char line[1024];
    tStation * head = NULL;
    tStation * station;

    FILE * inputFile;

    inputFile = stdin;
    if ( argc == 2 )
    {
        inputFile = fopen( argv[1], "r" );
        if ( inputFile == NULL)
        {
            perror( "unable to open file" );
            exit(2);
        }
    }

    while ( fgets( line, sizeof( line ), inputFile ) != NULL)
    {
        station = calloc( 1, sizeof( tStation ));
        if ( station != NULL)
        {
            processLine( line, station );

            tStation * stn  = head;
            tStation * prev = NULL;

            while ( stn != NULL && strcmp( stn->callsign, station->callsign ) != 0 )
            {
                prev = stn;
                stn  = stn->next;
            }
            if ( stn == NULL)
            {
                if ( prev != NULL )
                {
                    /* add to end of the list */
                    station->next = prev->next;
                    prev->next    = station;
                }
                else
                {
                    /* add first entry to empty list */
                    station->next = NULL;
                    head          = station;
                }
            }
        }
    }

    FILE * hashFile = fopen(kHashFilename, "w");
    if (hashFile != NULL) {
        fprintf(hashFile, "%s", kHashPrefix);
        for (station = head; station != NULL; station = station->next)
        {
            fprintf(hashFile, "\t\"%s\"%c\n", station->callsign, station->next != NULL ? ',' : ' ');
        }
        fprintf(hashFile, "%s", kHashSuffix);
        fclose(hashFile);
    }

    FILE * headerFile = fopen( kHeaderFilename, "w" );
    if (headerFile != NULL)
    {
        fprintf( headerFile, "%s", kHeaderPrefix);
        for (station = head; station != NULL; station = station->next) {
            fprintf(headerFile, "\t[kUSCallsign%-4s] = { \"%s\"%*c %2d, \"%s\"%*c kUSState%s }%c\n", station->callsign,
                    station->callsign, (int)(strlen(station->callsign) - 5), ',', station->logicalChan, station->city,
                    (int)(strlen(station->city) - 20), ',', station->state, station->next != NULL ? ',' : ' ');
        }
        fprintf( headerFile, "%s", kHeaderSuffix);
        fclose(headerFile);
    }

    return result;
}