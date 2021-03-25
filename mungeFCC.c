//
// Created by paul on 11/8/20.
//

#include "mungeFCC.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

const char * kPrefix = "struct FCCStationData[] = \n{\n";
const char * kSuffix = "};\n";

#define ServiceType( l1, l2, l3 ) (( l1 << 16) | (l2 << 8) | l3)

typedef enum
{
    kServiceUnSet = 0,
    kServiceLPA = ServiceType( 'L', 'P', 'A' ),
    kServiceLPD = ServiceType( 'L', 'P', 'D' ),
    kServiceLPT = ServiceType( 'L', 'P', 'T' ),
    kServiceLPX = ServiceType( 'L', 'P', 'X' )
} tServiceType;

typedef struct pStation {
    struct pStation * next;
    const char * callsign;
    tServiceType service;
    const char * rfChan;
    const char * logicalChan;
    const char * city;
    const char * state;
    const char * country;
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

tServiceType decodeService( char * service )
{
    unsigned int serviceType;
    unsigned char * p = (unsigned char *) service;

    serviceType = *p++;
    serviceType = (serviceType << 8) | *p++;
    serviceType = (serviceType << 8) | *p;
    return (tServiceType)serviceType;
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
        case 2: // station callsign
            {
                char * dash = strchr( field, '-' );
                if ( dash != NULL) { *dash = '\0'; }

                station->callsign = strdup( field );
            }
            break;

        case 4: // station type
            station->service = decodeService( field );
            break;

        case 11: // city
            toTitleCase( field );
            station->city = strdup( field );
            break;

        case 12: // state
            station->state = strdup( field );
            break;

        case 13: // country
            station->country = strdup( field );
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

            switch ( station->service )
            {
            case kServiceLPA:
            case kServiceLPD:
            case kServiceLPT:
            case kServiceLPX:
                break;

            default:
                {
                    tStation * stn = head;
                    while ( stn != NULL)
                    {
                        if ( strcmp( stn->callsign, station->callsign ) == 0 )
                             { break; }
                        else { stn = stn->next; }
                    }
                    if ( stn == NULL)
                    {
                        station->next = head;
                        head = station;
                    }
                }
                break;
            }
        }
    }

    printf( "%s", kPrefix );
    for ( station = head; station != NULL; station = station->next )
    {
        fprintf( stdout, "\t{ \"%s\", \"%s\", \"%s\", \"%s\" },\n",
                         station->callsign,
                         station->city,
                         station->state,
                         station->country );
    }
    printf( "%s", kSuffix );

    return result;
}