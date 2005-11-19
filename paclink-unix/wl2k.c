#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <stdio.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#include "strupper.h"
#include "wl2k.h"
#include "timeout.h"
#include "midseen.h"
#include "buffer.h"

#define PROPLIMIT 5
#define WL2KBUF 2048
#define WL2K_TEMPFILE_TEMPLATE "/tmp/wl2k.XXXXXX"
#define PENDING LOCALSTATEDIR "/wl2k/pending"

struct proposal {
  char code;
  char type;
  char mid[13];
  unsigned long usize;
  unsigned long csize;
  struct proposal *next;
  char *path;
  unsigned char *cdata;
  char *title;
  unsigned long offset;
  int accepted;
  int delete;
};

static int getrawchar(FILE *fp);
static int getcompressed(FILE *fp, FILE *ofp);
static struct proposal *parse_proposal(char *propline);
static int b2outboundproposal(FILE *fp, char *lastcommand, struct proposal **oproplist);
static void printprop(struct proposal *prop);
static void putcompressed(struct proposal *prop, FILE *fp);
static char *getline(FILE *fp, int terminator);
static void dodelete(struct proposal **oproplist, struct proposal **nproplist);

static int
getrawchar(FILE *fp)
{
  int c;

  resettimeout();
  c = fgetc(fp);
  if (c == EOF) {
    fprintf(stderr, "lost connection in getrawchar()\n");
    exit(EXIT_FAILURE);
  }
  return c;
}

#define CHRNUL 0
#define CHRSOH 1
#define CHRSTX 2
#define CHREOT 4

static int
getcompressed(FILE *fp, FILE *ofp)
{
  int c;
  int len;
  int i;
  unsigned char title[81];
  unsigned char offset[7];
  int cksum = 0;

  c = getrawchar(fp);
  if (c != CHRSOH) {
    return WL2K_COMPRESSED_BAD;
  }
  len = getrawchar(fp);
  title[80] = '\0';
  for (i = 0; i < 80; i++) {
    c = getrawchar(fp);
    len--;
    title[i] = c;
    if (c == CHRNUL) {
      ungetc(c, fp);
      len++;
      break;
    }
  }
  c = getrawchar(fp);
  len--;
  if (c != CHRNUL) {
    return WL2K_COMPRESSED_BAD;
  }
  printf("title: %s\n", title);
  offset[6] = '\0';
  for (i = 0; i < 6; i++) {
    c = getrawchar(fp);
    len--;
    offset[i] = c;
    if (c == CHRNUL) {
      ungetc(c, fp);
      len++;
      break;
    }
  }
  c = getrawchar(fp);
  len--;
  if (c != CHRNUL) {
    return WL2K_COMPRESSED_BAD;
  }
  printf("offset: %s\n", offset);
  if (len != 0) {
    return WL2K_COMPRESSED_BAD;
  }
  if (strcmp(offset, "0") != 0) {
    return WL2K_COMPRESSED_BAD;
  }

  for (;;) {
    c = getrawchar(fp);
    switch (c) {
    case CHRSTX:
      printf("STX\n");
      len = getrawchar(fp);
      if (len == 0) {
	len = 256;
      }
      printf("len %d\n", len);
      while (len--) {
	c = getrawchar(fp);
	if (fputc(c, ofp) == EOF) {
	  perror("fputc()");
	  return WL2K_COMPRESSED_BAD;
	}
	cksum = (cksum + c) % 256;
      }
      break;
    case CHREOT:
      printf("EOT\n");
      c = getrawchar(fp);
      cksum = (cksum + c) % 256;
      if (cksum != 0) {
	fprintf(stderr, "bad cksum\n");
	return WL2K_COMPRESSED_BAD;
      }
      return WL2K_COMPRESSED_GOOD;
      break;
    default:
      fprintf(stderr, "unexpected character in compressed stream\n");
      return WL2K_COMPRESSED_BAD;
      break;
    }
  }
  return WL2K_COMPRESSED_BAD;
}

static void
putcompressed(struct proposal *prop, FILE *fp)
{
  int len;
  int i;
  unsigned char title[81];
  unsigned char offset[7];
  int cksum = 0;
  char *cp;
  long rem;

  strlcpy(title, prop->title, sizeof(title));
  snprintf(offset, sizeof(offset), "%lu", prop->offset);

  printf("transmitting [%s] [offset %s]\n", title, offset);

  len = strlen(title) + strlen(offset) + 2;
  fprintf(fp, "%c%c%s%c%s%c", CHRSOH, len, title, CHRNUL, offset, CHRNUL);

  rem = prop->csize;
  cp = prop->cdata;

  if (rem < 6) {
    fprintf(stderr, "invalid compressed data\n");
    exit(EXIT_FAILURE);
  }
  fprintf(fp, "%c%c", CHRSTX, 6);
  for (i = 0; i < 6; i++) {
    cksum += *cp;
    fputc(*cp++, fp);
  }
  rem -= 6;

  cp += prop->offset;
  rem -= prop->offset;

  if (rem < 0) {
    fprintf(stderr, "invalid offset\n");
    exit(EXIT_FAILURE);
  }

  while (rem > 0) {
    printf("... %ld\n", rem);
    if (rem > 250) {
      len = 250;
    } else {
      len = rem;
    }
    fprintf(fp, "%c%c", CHRSTX, len);
    while (rem--) {
      cksum += *cp;
      fputc(*cp++, fp);
      len--;
    }
  }

  cksum = -cksum & 0xff;
  fprintf(fp, "%c%c", CHREOT, cksum);
}

static struct proposal *
parse_proposal(char *propline)
{
  char *cp = propline;
  static struct proposal prop;
  int i;
  char *endp;

  if (!cp) {
    return NULL;
  }
  if (*cp++ != 'F') {
    return NULL;
  }
  prop.code = *cp++;
  switch (prop.code) {
  case 'C':
    if (*cp++ != ' ') {
      fprintf(stderr, "malformed proposal 1\n");
      return NULL;
    }
    prop.type = *cp++;
    if ((prop.type != 'C') && (prop.type != 'E')) {
      fprintf(stderr, "malformed proposal 2\n");
      return NULL;
    }
    if (*cp++ != 'M') {
      fprintf(stderr, "malformed proposal 3\n");
      return NULL;
    }
    if (*cp++ != ' ') {
      fprintf(stderr, "malformed proposal 4\n");
      return NULL;
    }
    for (i = 0; i < 12; i++) {
      prop.mid[i] = *cp++;
      if (prop.mid[i] == ' ') {
	prop.mid[i] = '\0';
	cp--;
	break;
      } else {
	if (prop.mid[i] == '\0') {
	  fprintf(stderr, "malformed proposal 5\n");
	  return NULL;
	}
      }
    }
    prop.mid[12] = '\0';
    if (*cp++ != ' ') {
      fprintf(stderr, "malformed proposal 6\n");
      return NULL;
    }
    prop.usize = strtoul(cp, &endp, 10);
    cp = endp;
    if (*cp++ != ' ') {
      fprintf(stderr, "malformed proposal 7\n");
      return NULL;
    }
    prop.csize = (unsigned int) strtoul(cp, &endp, 10);
    cp = endp;
    if (*cp != ' ') {
      fprintf(stderr, "malformed proposal 8\n");
      return NULL;
    }
    break;
  case 'A':
  case 'B':
  default:
    prop.type = 'X';
    prop.mid[0] = '\0';
    prop.usize = 0;
    prop.csize = 0;
    break;
    fprintf(stderr, "unsupported proposal type %c\n", prop.code);
    break;
  }
  prop.next = NULL;
  prop.path = NULL;

  return &prop;
}

static void
printprop(struct proposal *prop)
{
  printf("proposal code %c type %c mid %s usize %lu csize %lu next %p path %s cdata %p\n",
	 prop->code,
	 prop->type,
	 prop->mid,
	 prop->usize,
	 prop->csize,
	 prop->next,
	 prop->path,
	 prop->cdata);
}

static void
dodelete(struct proposal **oproplist, struct proposal **nproplist)
{
  if ((oproplist == NULL) || (nproplist == NULL)) {
    fprintf(stderr, "bad call to dodelete()\n");
    exit(EXIT_FAILURE);
  }
  while (*oproplist != *nproplist) {
    if (((*oproplist)->delete) && ((*oproplist)->path)) {
      printf("DELETING PROPOSAL: ");
      printprop(*oproplist);
      unlink((*oproplist)->path);
    }
    oproplist = &((*oproplist)->next);
  }
}

static struct proposal *
prepare_outbound_proposals(void)
{
  struct proposal *prop;
  struct proposal **opropnext;
  struct proposal *oproplist = NULL;
  DIR *dirp;
  struct dirent *dp;
  struct stat sb;
  char *sfn;
  FILE *sfp;
  char *tfn;
  char *cmd;
  char *line;
  unsigned char *cp;

  opropnext = &oproplist;
  if ((dirp = opendir(PENDING)) == NULL) {
    perror("opendir()");
    exit(EXIT_FAILURE);
  }
  while ((dp = readdir(dirp)) != NULL) {
    if (dp->d_type != DT_REG) {
      continue;
    }
    if (strlen(dp->d_name) > 12) {
      fprintf(stderr,
	      "warning: skipping bad filename %s in pending directory %s\n",
	      dp->d_name, PENDING);
      continue;
    }
    printf("%s\n", dp->d_name);
    if ((prop = malloc(sizeof(struct proposal))) == NULL) {
      perror("malloc()");
      exit(EXIT_FAILURE);
    }
    prop->code = 'C';
    prop->type = 'E';
    strlcpy(prop->mid, dp->d_name, 13);
    if (asprintf(&prop->path, "%s/%s", PENDING, dp->d_name) == -1) {
      perror("asprintf()");
      exit(EXIT_FAILURE);
    }

    if (stat(prop->path, &sb) != 0) {
      perror("stat()");
      exit(EXIT_FAILURE);
    }

    prop->usize = (unsigned long) sb.st_size;

    if ((sfn = strdup(WL2K_TEMPFILE_TEMPLATE)) == NULL) {
      perror("strdup()");
      exit(EXIT_FAILURE);
    }

    /* XXX */
    if ((tfn = mktemp(sfn)) == NULL) {
      perror(sfn);
      exit(EXIT_FAILURE);
    }

    /* XXX */
    if (asprintf(&cmd, "./lzhuf_1 e1 %s %s", prop->path, tfn) == -1) {
      perror("asprintf()");
      exit(EXIT_FAILURE);
    }
    if (system(cmd) != 0) {
      fprintf(stderr, "error uncompressing received data\n");
      exit(EXIT_FAILURE);
    }
    free(cmd);

    if (stat(tfn, &sb) != 0) {
      perror("stat()");
      exit(EXIT_FAILURE);
    }
    prop->csize = (unsigned long) sb.st_size;
    if ((prop->cdata = malloc(prop->csize * sizeof(unsigned char))) == NULL) {
      perror("malloc()");
      exit(EXIT_FAILURE);
    }

    if ((sfp = fopen(tfn, "r")) == NULL) {
      perror("fopen()");
      exit(EXIT_FAILURE);
    }

    printf("sfp %p prop->path %s tfn %s\n", sfp, prop->path, tfn);

    if (fread(prop->cdata, prop->csize, 1, sfp) != 1) {
      perror("fread()");
      exit(EXIT_FAILURE);
    }
    fclose(sfp);
    unlink(tfn);
    free(sfn);

    prop->title = NULL;
    prop->offset = 0;
    prop->accepted = 0;
    prop->delete = 0;

    if ((sfp = fopen(prop->path, "r")) == NULL) {
      perror("fopen()");
      exit(EXIT_FAILURE);
    }

    while ((line = getline(sfp, '\n')) != NULL) {
      if (strncasecmp(line, "Subject:", 8) == 0) {
	if ((cp = strchr(line, '\r')) != NULL) {
	  *cp = '\0';
	}
	if ((cp = strchr(line, '\n')) != NULL) {
	  *cp = '\0';
	}
	cp = line + 8;
	while (isspace(*cp)) {
	  cp++;
	}
	if (strlen(cp) > 80) {
	  cp[80] = '\0';
	}
	prop->title = strdup(cp);
      }
    }
    fclose(sfp);

    if (prop->title == NULL) {
      prop->title = strdup("No subject");
    }

    prop->next = NULL;

    *opropnext = prop;
    opropnext = &prop->next;
  }
  closedir(dirp);

  printf("---\n");

  for (prop = oproplist; prop != NULL; prop = prop->next) {
    printprop(prop);
  }

  return oproplist;
}

static int
b2outboundproposal(FILE *fp, char *lastcommand, struct proposal **oproplist)
{
  int i;
  char *sp;
  unsigned char *cp;
  int cksum = 0;
  char *line;
  struct proposal *prop;
  char *endp;

  if (*oproplist) {
    prop = *oproplist;
    for (i = 0; i < PROPLIMIT; i++) {
      if (asprintf(&sp, "F%c %cM %s %lu %lu 0\r",
		  prop->code,
		  prop->type,
		  prop->mid,
		  prop->usize,
		  prop->csize) == -1) {
	perror("asprintf()");
	exit(EXIT_FAILURE);
      }
      printf(">%s\n", sp);
      fprintf(fp, "%s", sp);
      for (cp = sp; *cp; cp++) {
	cksum += (unsigned char) *cp;
      }
      free(sp);
      if ((prop = prop->next) == NULL) {
	break;
      }
    }
    cksum = -cksum & 0xff;
    printf(">F> %2X\n", cksum);
    fprintf(fp, "F> %2X\r", cksum);
    if ((line = wl2kgetline(fp)) == NULL) {
      fprintf(stderr, "connection closed\n");
      exit(EXIT_FAILURE);
    }
    printf("<%s\n", line);

    if (strncmp(line, "FS ", 3) != 0) {
      fprintf(stderr, "b2 protocol error\n");
      exit(EXIT_FAILURE);
    }
    prop = *oproplist;
    i = 0;
    cp = line + 3;
    while (*cp && isspace(*cp)) {
      cp++;
    }
    while (*cp && prop) {
      if (i == PROPLIMIT) {
	fprintf(stderr, "B2 protocol error\n");
	exit(EXIT_FAILURE);
      }
      prop->accepted = 0;
      prop->delete = 0;
      prop->offset = 0;
      switch(*cp) {
      case 'Y': case 'y':
      case 'H': case 'h':
      case '+':
	prop->accepted = 1;
	break;
      case 'N': case 'n':
      case 'R': case 'r':
      case '-':
	prop->delete = 1;
	break;
      case 'L': case 'l':
      case '=':
	break;
      case 'A': case 'a':
      case '!':
	prop->accepted = 1;
	prop->offset = strtoul(cp, &endp, 10);
	cp = endp - 1;
	break;
      default:
	fprintf(stderr, "B2 protocol error\n");
	exit(EXIT_FAILURE);
	break;
      }
      cp++;
      prop = prop->next;
    }

    prop = *oproplist;
    for (i = 0; i < PROPLIMIT; i++) {
      putcompressed(prop, fp);
      prop->delete = 1;
      if ((prop = prop->next) == NULL) {
	break;
      }
    }
    *oproplist = prop;
    return 0;
  } else if (strncmp(lastcommand, "FF", 2) == 0) {
    printf(">FQ\n");
    fprintf(fp, "FQ\r");
    return -1;
  } else {
    printf(">FF\n");
    fprintf(fp, "FF\r");
    return 0;
  }
}

static char *
getline(FILE *fp, int terminator)
{
  static struct buffer *buf = NULL;
  int c;

  if (buf == NULL) {
    if ((buf = buffer_new()) == NULL) {
      perror("buffer_new()");
      exit(EXIT_FAILURE);
    }
  } else {
    buffer_truncate(buf);
  }
  for (;;) {
    resettimeout();
    if ((c = fgetc(fp)) == EOF) {
      return NULL;
    }
    if (c == terminator) {
      if (buffer_addchar(buf, '\0') == -1) {
	perror("buffer_addchar()");
	exit(EXIT_FAILURE);
      }
      return buf->data;
    }
    if (buffer_addchar(buf, c) == -1) {
      perror("buffer_addchar()");
      exit(EXIT_FAILURE);
    }
  }
  return NULL;
}

char *
wl2kgetline(FILE *fp)
{

  return getline(fp, '\r');
}

void
wl2kexchange(char *mycall, char *yourcall, FILE *fp)
{
  char *cp;
  int proposals = 0;
  int proposalcksum = 0;
  int i;
  const char *sid = "[PaclinkUNIX-1.0-B2FHM]";
  char *inboundsid = NULL;
  char *inboundsidcodes = NULL;
  char *line;
  struct proposal *prop;
  struct proposal ipropary[PROPLIMIT];
  struct proposal *oproplist;
  struct proposal *nproplist;
  char *sfn;
  FILE *sfp;
  int fd = -1;
  char *cmd;
  unsigned long sentcksum;
  char *endp;
  int opropcount = 0;
  char responsechar;

  expire_mids();

  oproplist = prepare_outbound_proposals();

  for (prop = oproplist; prop; prop = prop->next) {
    opropcount++;
  }

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("<%s\n", line);
    if (line[0] == '[') {
      inboundsid = strdup(line);
      if ((cp = strrchr(inboundsid, '-')) == NULL) {
	fprintf(stderr, "bad sid %s\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      inboundsidcodes = strdup(cp);
      if ((cp = strrchr(inboundsidcodes, ']')) == NULL) {
	fprintf(stderr, "bad sid %s\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      *cp = '\0';
      strupper(inboundsidcodes);
      if (strstr(inboundsidcodes, "B2F") == NULL) {
	fprintf(stderr, "sid %s does not support B2F protocol\n", inboundsid);
	exit(EXIT_FAILURE);
      }
    } else if (line[strlen(line) - 1] == '>') {
      if (strchr(inboundsidcodes, 'I')) {
	printf(">; %s DE %s QTC %d\n", yourcall, mycall, opropcount);
	fprintf(fp, "; %s DE %s QTC %d\r", yourcall, mycall, opropcount);
      }
      printf(">%s\n", sid);
      fprintf(fp, "%s\r", sid);
      break;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Lost connection. 1\n");
    exit(EXIT_FAILURE);
  }

  nproplist = oproplist;
  if (b2outboundproposal(fp, line, &nproplist) != 0) {
    return;
  }

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("<%s\n", line);
    if (strncmp(line, ";", 1) == 0) {
      /* do nothing */
    } else if (strncmp(line, "FC", 2) == 0) {
      dodelete(&oproplist, &nproplist);
      for (cp = line; *cp; cp++) {
	proposalcksum += (unsigned char) *cp;
      }
      proposalcksum += '\r'; /* bletch */
      if (proposals == PROPLIMIT) {
	fprintf(stderr, "too many proposals\n");
	exit(EXIT_FAILURE);
      }
      if ((prop = parse_proposal(line)) == NULL) {
	fprintf(stderr, "failed to parse proposal\n");
	exit(EXIT_FAILURE);
      }
      memcpy(&ipropary[proposals], prop, sizeof(struct proposal));
      printprop(&ipropary[proposals]);
      proposals++;
    } else if (strncmp(line, "FF", 2) == 0) {
      dodelete(&oproplist, &nproplist);
      if (b2outboundproposal(fp, line, &nproplist) != 0) {
	return;
      }
    } else if (strncmp(line, "B", 1) == 0) {
      return;
    } else if (strncmp(line, "FQ", 2) == 0) {
      dodelete(&oproplist, &nproplist);
      return;
    } else if (strncmp(line, "F>", 2) == 0) {
      proposalcksum = (-proposalcksum) & 0xff;
      sentcksum = strtoul(line + 2, &endp, 16);

      if (sentcksum != (unsigned long) proposalcksum) {
	fprintf(stderr, "proposal cksum mismatch\n");
	exit(EXIT_FAILURE);
      }
      
      printf("%d proposals\n", proposals);

      if (proposals != 0) {
	printf("FS ");
	fprintf(fp, "FS ");
	for (i = 0; i < proposals; i++) {
	  ipropary[i].accepted = 0;
	  if (ipropary[i].code == 'C') {
	    if (check_mid(ipropary[i].mid)) {
	      responsechar = 'N';
	    } else {
	      responsechar = 'Y';
	      ipropary[i].accepted = 1;
	    }
	  } else {
	    responsechar = 'L';
	  }
	  putchar(responsechar);
	  putc(responsechar, fp);
	}
	printf("\n");
	fprintf(fp, "\r");

	for (i = 0; i < proposals; i++) {
	  if (ipropary[i].accepted != 1) {
	    continue;
	  }
	  if ((sfn = strdup(WL2K_TEMPFILE_TEMPLATE)) == NULL) {
	    perror("strdup()");
	    exit(EXIT_FAILURE);
	  }
	  if ((fd = mkstemp(sfn)) == -1 ||
	      (sfp = fdopen(fd, "w+")) == NULL) {
	    if (fd != -1) {
	      unlink(sfn);
	      close(fd);
	    }
	    perror(sfn);
	    exit(EXIT_FAILURE);
	  }

	  if (getcompressed(fp, sfp) != WL2K_COMPRESSED_GOOD) {
	    fprintf(stderr, "error receiving compressed data\n");
	    exit(EXIT_FAILURE);
	  }
	  if (fclose(sfp) != 0) {
	    fprintf(stderr, "error closing compressed data\n");
	    exit(EXIT_FAILURE);
	  }
	  printf("extracting...\n");
	  if (asprintf(&cmd, "./lzhuf_1 d1 %s %s", sfn, ipropary[i].mid) == -1) {
	    perror("asprintf()");
	    exit(EXIT_FAILURE);
	  }
	  if (system(cmd) != 0) {
	    fprintf(stderr, "error uncompressing received data\n");
	    exit(EXIT_FAILURE);
	  }
	  free(cmd);
	  unlink(sfn);
	  record_mid(ipropary[i].mid);
	  printf("Finished!\n");
#if 0
	  while ((line = wl2kgetline(fp)) != NULL) {
	    printf("%s\n", line);
	    if (line[0] == '\x1a') {
	      printf("yeeble\n");
	    }
	  }
	  if (line == NULL) {
	    fprintf(stderr, "Lost connection. 3\n");
	    exit(EXIT_FAILURE);
	  }
#endif
	}
      }
      proposals = 0;
      proposalcksum = 0;
      if (b2outboundproposal(fp, line, &nproplist) != 0) {
	return;
      }
    } else if (line[strlen(line - 1)] == '>') {
      dodelete(&oproplist, &nproplist);
      if (b2outboundproposal(fp, line, &nproplist) != 0) {
	return;
      }
    } else {
      fprintf(stderr, "unrecognized command: %s\n", line);
      exit(EXIT_FAILURE);
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Lost connection. 4\n");
    exit(EXIT_FAILURE);
  }
}
