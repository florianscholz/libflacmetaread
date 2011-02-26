#ifndef FLACMETAREAD_H_INCLUDED
#define FLACMETAREAD_H_INCLUDED

#include <stdio.h>


#define METABLOCK_STREAMINFO 0
#define METABLOCK_PADDING 1
#define METABLOCK_APPLICATION 2
#define METABLOCK_SEEKTABLE 3
#define METABLOCK_VORBIS 4
#define METABLOCK_CUESHEET 5
#define METABLOCK_PICTURE 6

#define METABLOCK_GET_VORBIS_LIST(x) (struct flac_meta_vorbis*)x->data
#define METABLOCK_GET_STREAMINFO(x) (struct flac_meta_streaminfo*)x->data

struct flac_file_structure {
    FILE *fp;
};


struct flac_meta_block_data {

};

struct flac_meta_streaminfo {
    unsigned int samplerate;
    unsigned int channels;
    unsigned int bits_per_sample;
};

struct flac_meta_block {
    unsigned char metablock_type;
    unsigned int metadata_length;
    void *data;
    struct flac_meta_block *next;
};

struct flac_meta_vorbis {
    unsigned int length;
    char* comment;
    struct flac_meta_vorbis *next;
};

_Bool flac_meta_open(const char* filename, struct flac_file_structure ** file_structure);
_Bool flac_meta_close(struct flac_file_structure **file_structure);
struct flac_meta_block *flac_meta_get_metablocks(struct flac_file_structure *file_structure);
void flac_meta_free_vorbis(struct flac_meta_vorbis* vorbis);
void flac_meta_free_streaminfo(struct flac_meta_streaminfo* streaminfo);
void flac_meta_free_blocks(struct flac_meta_block *block);
#endif // FLACMETAREAD_H_INCLUDED
