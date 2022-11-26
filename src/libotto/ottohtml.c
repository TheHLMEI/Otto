#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "otto.h"

typedef struct
{
   char *extension;
   char *mime_type;
} mime_map;

mime_map mime_types [] =
{
   {".css",  "text/css"},
   {".gif",  "image/gif"},
   {".htm",  "text/html"},
   {".html", "text/html"},
   {".jpeg", "image/jpeg"},
   {".jpg",  "image/jpeg"},
   {".ico",  "image/x-icon"},
   {".js",   "application/javascript"},
   {".pdf",  "application/pdf"},
   {".mp4",  "video/mp4"},
   {".png",  "image/png"},
   {".svg",  "image/svg+xml"},
   {".xml",  "text/xml"},
   {NULL,    NULL}
};

char *default_mime_type = "text/plain";

char *html_indentation[9] = {"",
                             "&nbsp;",
                             "&nbsp;&nbsp;",
                             "&nbsp;&nbsp;&nbsp;",
                             "&nbsp;&nbsp;&nbsp;&nbsp;",
                             "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                             "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                             "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;",
                             "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"};



void
ottohtml_parse_uri(ottohtml_query *q, char *uri)
{
   char *endpoint, *query, *amp, *val;
   char tmpstr[65535];

   // clear query
   memset(q, 0, sizeof(ottohtml_query));
   q->level = MAXLVL;

   // find last slash
   endpoint = strrchr(uri, '/');
   if(endpoint == NULL)
      endpoint = uri;
   else
      endpoint++;

   // look for start of query
   query = strchr(endpoint, '?');
   if(query != NULL)
   {
      *query = '\0';
   }

   // copy the endpoint to the structure
   strcpy(q->endpoint, endpoint);

   if(query != NULL)
   {
      *query = '?';
      query++;

      // now look for known parameters
      // get name
      if((val = strstr(query, "jobname=")) != NULL)
      {
         // copy to temp string
         strcpy(tmpstr, &val[8]);

         amp = strchr(tmpstr, '&');
         // null terminate
         if(amp != NULL)
         {
            *amp = '\0';
         }

         // copy the name
         if(strlen(tmpstr) > NAMLEN)
         {
            tmpstr[NAMLEN] = '\0';
         }
         strcpy(q->jobname, tmpstr);
      }
      // get level
      if((val = strstr(query, "level=")) != NULL)
      {
         // copy to temp string
         strcpy(tmpstr, &val[6]);

         amp = strchr(tmpstr, '&');
         // null terminate
         if(amp != NULL)
         {
            *amp = '\0';
         }

         // copy the level
         if(isdigit(tmpstr[0]))
            q->level = atoi(tmpstr);
      }

      // get status filters
      if((val = strstr(query, "IN=")) != NULL)
      {
         if(val[3] == '1') q->show_IN = OTTO_TRUE;
      }

      if((val = strstr(query, "AC=")) != NULL)
      {
         if(val[3] == '1') q->show_AC = OTTO_TRUE;
      }

      if((val = strstr(query, "OH=")) != NULL)
      {
         if(val[3] == '1') q->show_OH = OTTO_TRUE;
      }

      if((val = strstr(query, "RU=")) != NULL)
      {
         if(val[3] == '1') q->show_RU = OTTO_TRUE;
      }

      if((val = strstr(query, "SU=")) != NULL)
      {
         if(val[3] == '1') q->show_SU = OTTO_TRUE;
      }

      if((val = strstr(query, "FA=")) != NULL)
      {
         if(val[3] == '1') q->show_FA = OTTO_TRUE;
      }

      if((val = strstr(query, "TE=")) != NULL)
      {
         if(val[3] == '1') q->show_TE = OTTO_TRUE;
      }

      if((val = strstr(query, "BR=")) != NULL)
      {
         if(val[3] == '1') q->show_BR = OTTO_TRUE;
      }
   }

   // if no filters are on set them all on
   if( q->show_IN == OTTO_FALSE &&
       q->show_AC == OTTO_FALSE &&
       q->show_OH == OTTO_FALSE &&
       q->show_RU == OTTO_FALSE &&
       q->show_SU == OTTO_FALSE &&
       q->show_FA == OTTO_FALSE &&
       q->show_TE == OTTO_FALSE &&
       q->show_BR == OTTO_FALSE)
   {
      q->show_IN = OTTO_TRUE;
      q->show_AC = OTTO_TRUE;
      q->show_OH = OTTO_TRUE;
      q->show_RU = OTTO_TRUE;
      q->show_SU = OTTO_TRUE;
      q->show_FA = OTTO_TRUE;
      q->show_TE = OTTO_TRUE;
      q->show_BR = OTTO_TRUE;
   }
}



//
// ottohtml_send_error - send an error message to the client
//
void
ottohtml_send_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
   char content[1024];

   sprintf(content, "<html>\n"
                    "<head>\n"
                    "   <link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\">\n"
                    "   <link rel=\"stylesheet\" href=\"otto.css\" type=\"text/css\">\n"
                    "   <title>Otto HTTP Error</title>\n"
                    "</head>\n"
                    "<body>\n"
                    "%s: %s\n"
                    "<p>%s: %s\n"
                    "</body></html>\n", errnum, shortmsg, longmsg, cause);

   ottohtml_send(fd, errnum, shortmsg, "text/html", content, strlen(content));
}



//
// ottohtml_send - send any message to the client
//
void
ottohtml_send(int fd, char *msgnum, char *status, char *content_type, char *content, int contentlen)
{
   char buffer[32];
   char header[1024];

   sprintf(header, "HTTP/1.1 %s %s\n"
                   "Server: Otto HTTPD\n"
                   "Content-type: %s\n"
                   "Content-length: %d\n"
                   "\n", msgnum, status, content_type, contentlen);

   if (recv(fd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) != 0) 
   {
      send(fd, header, strlen(header), 0);
      send(fd, content, contentlen,    0);
   }
}


//
// ottohtml_send_attachment - send any generated output as file to the client
//
void
ottohtml_send_attachment(int fd, char *filename, char *content, int contentlen)
{
   char buffer[32];
   char header[1024];

   sprintf(header, "HTTP/1.1 200 OK\n"
                   "Server: Otto HTTPD\n"
                   "Content-type: Otto/Export\n"
                   "Content-length: %d\n"
                   "Content-Disposition: attachment; filename=\"%s\"\n"
                   "\n", contentlen, filename);

   if (recv(fd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT) != 0) 
   {
      send(fd, header, strlen(header), 0);
      send(fd, content, contentlen,    0);
   }
}


char *
get_mime_type(char *filename)
{
   char *dot = strrchr(filename, '.');
   if(dot)
   {
      mime_map *map = mime_types;
      while(map->extension)
      {
         if(strcmp(map->extension, dot) == 0)
         {
            return map->mime_type;
         }
         map++;
      }
   }
   return default_mime_type;
}



char *
get_html_indent(int level)
{

   if(level < 0) level = 0;
   if(level > 8) level = 8;

   return (html_indentation[level]);;
}





