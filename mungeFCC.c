//
// Created by paul on 11/8/20.
/*
 * Post-processes the 'facilities.dat' text file provided by the FCC
 * CDBS and converts it into source files that mungeM3U can compile
 * in and use to identify US TV Stations by callsign.
 *
 * mungeM3U needs the generated fccdata.hash, and fccdata.c files.
 * affiliates.hash is a file that can be found in both projects,
 * but is only similar, not identical.
 */

#include "mungeFCC.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libhashstrings.h>

#include "affiliate.h"

#define kHeaderFilename "fccdata.c"
const char * kHeaderPrefix = "struct FCCStationData[] = \n{\n";
const char * kHeaderSuffix = "};\n";

#define kHashFilename "fccdata.hash"
const char * kHashPrefix = "\n"
    "prefix = \"USCallsign\"\n"
    "mappings : {\n"
    "    ignoreCase = true\n"
    "    Separator = \" :|()[]-\"\n"
    "}\n\n"
    "keywords = [\n";

const char * kHashSuffix = "]\n";

/* extracted from the documents at https://fcc.gov/media/radio/cdbs-database-public-files */
typedef enum {
    comm_city,              /*  1: The city of the facility's "community served", city */
    comm_state,             /*  2: The state of the facility's "community served", state */
    eeo_rpt_ind,            /*  3: Indicates whether the station plans to or does employ five or more employees
                                   and therefore should submit equal employment opportunity reports, ind */
    fac_address1,           /*  4: The address of the facility, address */
    fac_address2,           /*  5: The address of the facility (continued), address */
    fac_callsign,           /*  6: The call sign of the facility/station, callsign */
    fac_channel,            /*  7: Channel number, int */
    fac_city,               /*  8: The city in which the facility is located. Also considered the Mailing City of the facility, city */
    fac_country,            /*  9: The country of the station, country */
    fac_frequency,          /* 10: The frequency assigned to the station, frequency (float) */
    fac_service,            /* 11: Identifies the service which the facility supports, char(2) */
    fac_state,              /* 12: The state in which the facility is located. The state of the mailing address. state */
    fac_status_date,        /* 13: The date the facility status took effect. datetime */
    fac_type,               /* 14: The type of the facility. varchar(3) */
    facility_id,            /* 15: Uniquely identifies a facility. int */
    lic_expiration_date,    /* 16: The date on which the FCC license or CP building permit expires. datetime */
    fac_status,             /* 17: The facility status contains the last status of the facility application processing.
                               It may be CP granted, license, granted, appeal pending, STA granted, silent without
                               STA, cancelled/deleted, etc. varchar(3) */
    fac_zip1,               /* 18: The First 5 digits of the Zipcode of the facility. char(5) */
    fac_zip2,               /* 19: The additional 4 digits of the Zipcode of the facility. char(4) */
    station_type,           /* 20: Identifies the station as a main or an auxiliary char(1) */

    assoc_facility_id,      /* 21: The facility ID "associated" with the FX station
                               (meaning, the facility_id that this FX station rebroadcasts), int */
    callsign_eff_date,      /* 22: The date the callsign became effective, datetime */
    tsid_ntsc,              /* 23: The assigned unique analog Transport Stream Identifier. int */
    tsid_dtv,               /* 24: The assigned unique digital Transport Stream Identifier. int */
    digital_status,         /* 25: The digital status of the facility, D for Digital, H for Hybrid., char(1) */
    sat_tv,                 /* 26: To designate satellite tv stations. char(1) */
    network_affil,	    /* 27: Current network affiliation (free text), if applicable. varchar(100) */
    nielsen_dma,	    /* 28: Nielsen DMA. varchar(60) */
    tv_virtual_channel,	    /* 29: TV Virtual Channel. int */
    last_change_date        /* 30: The date this record was last updated. datetime */
} tFacilityField;

const char * facilityFieldAsString[] = {
	[assoc_facility_id] = "assoc_facility_id",  /* The facility ID "associated" with the FX station
	                                                (meaning, the facility_id that this FX station rebroadcasts), int */
	[callsign_eff_date] = "callsign_eff_date",  /*  The date the callsign became effective, datetime */
	[comm_city] = "comm_city",                  /* The city of the facility's "community served", city */
	[comm_state] = "comm_state",                /* The state of the facility's "community served", state */
	[digital_status] = "digital_status",        /* The digital status of the facility, D for Digital, H for Hybrid., char(1) */
	[eeo_rpt_ind] = "eeo_rpt_ind",              /* Indicates whether the station plans to or does employ five or more employees
                                                       and therefore should submit equal employment opportunity reports, ind */
	[fac_address1] = "fac_address1",            /* The address of the facility, address */
	[fac_address2] = "fac_address2",            /* The address of the facility (continued), address */
	[fac_callsign] = "fac_callsign",            /* The call sign of the facility/station, callsign */
	[fac_channel] = "fac_channel",              /* Channel number, int */
	[fac_city] = "fac_city",                    /* The city in which the facility is located. Also considered the Mailing City of the facility, city */
	[fac_country] = "fac_country",              /* The country of the station, country */
	[fac_frequency] = "fac_frequency",          /* The frequency assigned to the station, frequency */
	[fac_service] = "fac_service",              /* Identifies the service which the facility supports, char(2) */
	[fac_state] = "fac_state",                  /* The state in which the facility is located. The state of the mailing address. state */
	[fac_status] = "fac_status",                /* The facility status contains the last status of the facility application processing.
                                                       It may be CP granted, license, granted, appeal pending, STA granted, silent without
	                                               STA, cancelled/deleted, etc. varchar(3) */
	[fac_status_date] = "fac_status_date",      /* The date the facility status took effect. datetime */
	[fac_type] = "fac_type",                    /* The type of the facility. varchar(3) */
	[fac_zip1] = "fac_zip1",                    /* The First 5 digits of the Zipcode of the facility. char(5) */
	[fac_zip2] = "fac_zip2",                    /* The additional 4 digits of the Zipcode of the facility. char(4) */
	[facility_id] = "facility_id",              /* Uniquely identifies a facility. int */
	[last_change_date] = "last_change_date",    /* The date this record was last updated. datetime */
	[lic_expiration_date] = "lic_expiration_date", /* The date on which the FCC license or CP building permit expires. datetime */
	[network_affil] = "network_affil",          /* Current network affiliation (free text, not from a list of values), if applicable. varchar(100) */
	[nielsen_dma] = "nielsen_dma",              /* Nielsen DMA. varchar(60) */
	[sat_tv] = "sat_tv",                        /* To designate satellite tv stations. char(1) */
	[station_type] = "station_type",            /* Identifies the station as a main or an auxiliary char(1) */
	[tsid_dtv] = "tsid_dtv",                    /* The assigned unique digital Transport Stream Identifier. int */
	[tsid_ntsc] = "tsid_ntsc",                  /* The assigned unique analog Transport Stream Identifier. int */
        [tv_virtual_channel] = "tv_virtual_channel" /* TV Virtual Channel. int */
};

typedef struct sStation {
    struct sStation * next;
    const char *      callsign;
    tIndex            affiliate;
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
    char   field[256];
    char * fp;
    int    fl;
    char * r;

    fieldNum = 0;
    while ( *p != '\0' )
    {
        fp = field;
        fl = sizeof( field );

        r  = field;
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

        *r = '\0';


        switch ( fieldNum )
        {
        case fac_type:
            /* if it's not 'CDT' (commercial digital television) or
             * 'EDT' (educational digital television) then ignore it */
            if (strcasecmp("CDT", field ) != 0
                && strcasecmp("EDT", field) != 0 )
            {
                // fprintf(stderr, "%s - %s: \"%s\"\n", station->callsign, facilityFieldAsString[fieldNum], field);
                return -1;
            }
            break;

        case fac_service: /* if it's not 'DT' then ignore it */
            if (strcasecmp("DT", field) != 0) {
                // fprintf(stderr, "%s - %s: \"%s\"\n", station->callsign, facilityFieldAsString[fieldNum], field);
                return -1;
            }
            break;


        case fac_callsign: // station callsign
            {
                char * dash = strchr( field, '-' );
                if ( dash != NULL) { *dash = '\0'; }

                if (strlen(field) < 3)
                {
                    return -1;
                }

                station->callsign = strdup( field );
            }
            break;

        case fac_status:
            if (strcasecmp("LICEN", field) != 0) {
                //fprintf(stderr, "%s - %s: \"%s\"\n", station->callsign, facilityFieldAsString[fieldNum], field);
                return -1;
            }
            break;

        case network_affil:
            {
                tHash hash = 0;
                const char * p = field;
                while (*p != '\0')
                {
                    hash = hashChar(hash, remapChar( gAffiliateCharMap, *p++ ));
                }
                station->affiliate = findHash(mapAffiliateSearch, hash );
#if 0
                if ( station->affiliate == kAffiliateUnknown )
                {
                    fprintf( stderr, "unknown: \"%s\"\n", field );
                }
#endif
            }
            break;

        case comm_city: // city
            toTitleCase( field );
            station->city = strdup( field );
            break;

        case comm_state: // state
            station->state = strdup( field );
            break;

        case tv_virtual_channel: // logical channel
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

void clearStation( tStation * station )
{
    station->next = NULL;
    station->logicalChan = 0;
    station->affiliate = 0;

    if (station->callsign != NULL)
    {
        free( (void *)station->callsign);
        station->callsign = NULL;
    }
    if (station->city != NULL) {
        free( (void *)station->city);
        station->city = NULL;
    }
    if (station->state != NULL) {
        free( (void *)station->state);
        station->state = NULL;
    }
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

    station = calloc( 1, sizeof( tStation ));
    while (station != NULL && fgets(line, sizeof(line), inputFile) != NULL)
    {
        if ( processLine( line, station ) != 0 )
        {
            /* not worthy - release any allocated strings */
            clearStation( station );
        }
        else
        {
            tStation *stn = head;
            tStation *prev = NULL;

            while (stn != NULL && strcmp(stn->callsign, station->callsign) != 0) {
                prev = stn;
                stn = stn->next;
            }
            if (stn != NULL)
            {   /* it's already in the list */
                clearStation(station);
            }
            else
            {   /* append to the list */
                if (prev != NULL) {
                    /* add to end of the list */
                    station->next = prev->next;
                    prev->next = station;
                } else {
                    /* add first entry to empty list */
                    station->next = NULL;
                    head = station;
                }
                /* get a fresh one */
                station = calloc(1, sizeof(tStation));
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
            fprintf(headerFile, "\t[kUSCallsign%-4s] = { \"%s\"%*c kAffiliate%s, %2d, \"%s\"%*c kUSState%s }%c\n",
                    station->callsign, station->callsign, (int)(strlen(station->callsign) - 5), ',',
                    lookupAffiliateAsString[station->affiliate],
                    station->logicalChan,
                    station->city,
                    (int)(strlen(station->city) - 20), ',', station->state, station->next != NULL ? ',' : ' ');
        }
        fprintf( headerFile, "%s", kHeaderSuffix);
        fclose(headerFile);
    }

    return result;
}