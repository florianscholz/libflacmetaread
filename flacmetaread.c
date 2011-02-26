#include "flacmetaread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LittleEndian(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))

static unsigned int inline get_metablock_type(unsigned int data)
{
    return (LittleEndian(data) >> 24) & 0b01111111;
}
static unsigned int inline get_metablock_length(unsigned int data)
{
    return LittleEndian(data) & 0x0FFF;
}
static unsigned int inline get_last_block_bit(unsigned int data)
{
    return LittleEndian(data) >> 31;
}
static unsigned int inline get_samplerate(unsigned int data)
{
    return LittleEndian(data) >> 12;
}
static unsigned int inline get_channels(unsigned int data)
{
    return ((LittleEndian(data) >> 9) & 0b111) + 1;
}
static unsigned int inline get_bits_per_sample(unsigned int data)
{
    return ((LittleEndian(data) >> 4) & 0b11111) + 1;
}

/* Diese Funktion verarbeitet den STREAMINFO (FLAC-SPEZIFIKATION) Header und kopiert die wichtigsten Informationen in die flac_meta_streaminfo Struktur */

static struct flac_meta_streaminfo* flac_meta_parse_streaminfo(struct flac_file_structure *file_structure)
{
    struct flac_meta_streaminfo *streaminfo = (struct flac_meta_streaminfo*)malloc(sizeof(struct flac_meta_streaminfo));
    unsigned int data;

    fseek(file_structure->fp,10,SEEK_CUR); //Maximale und Minimale Längen der Frames
    fread(&data,4,1,file_structure->fp);

    streaminfo->samplerate = get_samplerate(data);
    streaminfo->channels = get_channels(data);
    streaminfo->bits_per_sample = get_bits_per_sample(data);

    fseek(file_structure->fp,20,SEEK_CUR);

    return streaminfo;
}
/* Diese Funktion gibt den Speicher, der durch eine flac_meta_streaminfo Struktur benötigt reserviert wurde, wieder frei */
void flac_meta_free_streaminfo(struct flac_meta_streaminfo* streaminfo)
{
    free(streaminfo);
}
/* Diese Funktion gibt den Speicher frei, der durch eine Vorbis-Tag-Liste reserviert wurde. */
void flac_meta_free_vorbis(struct flac_meta_vorbis* vorbis)
{
    struct flac_meta_vorbis *temporary;
    struct flac_meta_vorbis *walker;

    walker = vorbis;
    while(walker != NULL)
    {
        temporary = walker;
        walker = walker->next;

        free(temporary->comment);
        free(temporary);
    }
}
/* Diese Funktion gibt den Speicher frei, der durch eine Meta-Block-Liste reserivert wurde */
void flac_meta_free_blocks(struct flac_meta_block *block)
{
    struct flac_meta_block *temporary;
    struct flac_meta_block *walker;

    walker = block;
    while(walker != NULL)
    {
        temporary = walker;
        walker = walker->next;

        free(temporary);
    }
}
/* Diese Funktion liest eine Vorbis-Tag-Liste ein */
static struct flac_meta_vorbis* flac_meta_parse_vorbis(struct flac_file_structure *file_structure)
{
    unsigned int vendor_length;
    unsigned int list_length;
    int i = 0;

    struct flac_meta_vorbis *vorbis_comment = NULL;
    struct flac_meta_vorbis *vorbis_current = NULL;

    fread(&vendor_length,4,1,file_structure->fp);
    fseek(file_structure->fp,vendor_length,SEEK_CUR);
    fread(&list_length,4,1,file_structure->fp);

    for(i=0;i<list_length;i++)
    {
        unsigned comment_length;
        char* comment_content;

        fread(&comment_length,4,1,file_structure->fp);
        comment_content = (char*)malloc(comment_length);
        fread(comment_content,1,comment_length,file_structure->fp);

        if(vorbis_comment == NULL)
        {
            vorbis_comment = (struct flac_meta_vorbis*)malloc(sizeof(struct flac_meta_vorbis));
            vorbis_current = vorbis_comment;
        }
        else
        {
            for(vorbis_current = vorbis_comment; vorbis_current->next != NULL; vorbis_current = vorbis_current->next);

            vorbis_current->next = (struct flac_meta_vorbis*)malloc(sizeof(struct flac_meta_vorbis));
            vorbis_current = vorbis_current->next;
        }

        vorbis_current->comment = comment_content;
        vorbis_current->length = comment_length;
        vorbis_current->next = NULL;

    }

    return vorbis_comment;
}

/* Diese Funktion liest eine FLAC-Datei ein und speichert ein Handle im Zeiger-Zeiger einer flac_file_structure Struktur */
_Bool flac_meta_open(const char* filename, struct flac_file_structure ** file_structure)
{
    FILE *fp = fopen(filename,"rb");

    if(fp == NULL)
    {
        return 0;
    }
    else
    {
        char identity_tag[4];

        if(ferror(fp) != 0)
        {
            *file_structure = NULL;
            fclose(fp);
            return 0;
        }

        fread(identity_tag,1,4,fp);

        if(ferror(fp) != 0)
        {
            *file_structure = NULL;
            fclose(fp);
            return 0;
        }
        else if(strncmp(identity_tag,"fLaC",4))
        {
            *file_structure = NULL;
            fclose(fp);
            return 0;
        }
        else
        {

            (*file_structure) = (struct flac_file_structure*)malloc(sizeof(struct flac_file_structure));
            (*file_structure)->fp = fp;
            return 1;
        }
    }
}
/* Diese Funktion gibt den Speicher einer flac_file_structure Struktur frei, die durch den übergebenen file_structure Zeiger adressiert wurde */
_Bool flac_meta_close(struct flac_file_structure **file_structure)
{
    if(*file_structure != NULL)
    {
        if((*file_structure)->fp != NULL)
        {
            fclose((*file_structure)->fp);
        }

        free(*file_structure);

        return 1;
    }
    else
        return 0;
}
/* Diese Funktion ist die Hauptroutine, die die einzelnen Blöcke aus der, durch den flac_file_structure Zeiger file_structure verwiesenen FLAC-Datei ausliest und analysiert*/
struct flac_meta_block *flac_meta_get_metablocks(struct flac_file_structure *file_structure)
{
    unsigned int data = 0;

    struct flac_meta_block *meta_block = NULL;
    struct flac_meta_block *current_meta_block;

    do
    {

        if(meta_block != NULL)
        {

            for(current_meta_block = meta_block; current_meta_block->next != NULL; current_meta_block = current_meta_block->next);

            current_meta_block->next = (struct flac_meta_block *)malloc(sizeof(struct flac_meta_block));
            current_meta_block = current_meta_block->next;
        }
        else
        {
            meta_block = (struct flac_meta_block *)malloc(sizeof(struct flac_meta_block));
            current_meta_block = meta_block;
        }

        fread(&data,4,1,file_structure->fp);

        current_meta_block->metablock_type = get_metablock_type(data);
        current_meta_block->metadata_length = get_metablock_length(data);
        current_meta_block->next = NULL;

        switch(current_meta_block->metablock_type)
        {
            case METABLOCK_STREAMINFO:
                current_meta_block->data = (void*)flac_meta_parse_streaminfo(file_structure);
                break;
            case METABLOCK_VORBIS:
                current_meta_block->data = (void*)flac_meta_parse_vorbis(file_structure);
                break;
            default:
                fseek(file_structure->fp,current_meta_block->metadata_length,SEEK_CUR);
        }

    }
    while(get_last_block_bit(data) != 1);

    return meta_block;
}

