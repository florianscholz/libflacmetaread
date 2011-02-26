#include "flacmetaread.h"

#include <stdlib.h>
#include <limits.h>

int main(int argc, char* argv[])
{
   struct flac_file_structure *file_structure;
   struct flac_meta_block *current_metablock, *metablocks;
   struct flac_meta_vorbis *current_vorbis, *vorbis_tags;
   char path[4096];

   if(argc <= 1)
   {
      printf("error: invalid parameter count\n");
      return -1;
   }
   else
   {
      if(realpath(argv[1],path) == NULL)
      {
         printf("error: invalid path \"%s\"\n",argv[1]);
         return -1;
      }
   }

   if(flac_meta_open(path,&file_structure) != 1)
   {
      printf("error: can't open flac file\n");
      goto ende;
   }

   metablocks = flac_meta_get_metablocks(file_structure);

   for(current_metablock=metablocks;current_metablock != NULL; current_metablock = current_metablock->next)
   {
      switch(current_metablock->metablock_type)
      {
         case METABLOCK_STREAMINFO:
            printf("*** Streaming Header Information ***\n");

            struct flac_meta_streaminfo *streaminfo = METABLOCK_GET_STREAMINFO(current_metablock);

            printf("samplerate:\t%d hz\n",streaminfo->samplerate);
            printf("bit depth:\t%d bits\n",streaminfo->bits_per_sample);
            printf("channels:\t%d\n",streaminfo->channels);

            flac_meta_free_streaminfo(streaminfo);
         break;
         case METABLOCK_VORBIS:
            printf("*** Vorbis Header Information ******\n");

            vorbis_tags = METABLOCK_GET_VORBIS_LIST(current_metablock);

            for(current_vorbis = vorbis_tags; current_vorbis != NULL; current_vorbis = current_vorbis->next)
               printf("identification tag: %.*s\n",current_vorbis->length,current_vorbis->comment);

            flac_meta_free_vorbis(vorbis_tags);
         break;
      }
   }

   flac_meta_free_blocks(metablocks);

   ende:
   flac_meta_close(&file_structure);

   return 0;
}
