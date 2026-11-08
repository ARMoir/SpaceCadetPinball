/* Original high_score.cpp, adapted from a configuration-backed 5-entry
 * desktop table to a tiny local DOS text file (SCORES.DAT). No gameplay I/O.
 * Invalid/partial files reset the table without crashing or changing rules. */
#include "scores.h"
#include <stdio.h>
#include <string.h>

ScoreEntry scores_table[SCORE_COUNT];

void scores_reset(void)
{
    int i;
    for(i=0;i<SCORE_COUNT;i++) {
        strcpy(scores_table[i].initials,"---");
        scores_table[i].score=0;
    }
}

static int valid_name(const char *s)
{
    int i;
    if(!s) return 0;
    for(i=0;i<3;i++)
        if(s[i]<'A' || s[i]>'Z') return 0;
    return s[3]==0;
}

int scores_rank(unsigned long score)
{
    int i;
    if(!score) return -1;
    for(i=0;i<SCORE_COUNT;i++)
        if(score>scores_table[i].score) return i;
    return -1; /* Stable ties only qualify if a free (zero) row exists. */
}

int scores_insert(unsigned long score,const char *initials)
{
    int i,rank=scores_rank(score);
    if(rank<0 || !valid_name(initials)) return -1;
    for(i=SCORE_COUNT-1;i>rank;i--)
        scores_table[i]=scores_table[i-1];
    strcpy(scores_table[rank].initials,initials);
    scores_table[rank].score=score;
    return rank;
}

/* File: SC1 LF, then five fixed-width "AAA 0000000000 LF" lines.
 * Text-mode CRT translates LF/CRLF on DOS automatically.
 * Do not load partial or unsorted files into the live score table. */
int scores_load(const char *path)
{
    FILE *fp;
    char line[24];
    int i,j,ok=1;
    unsigned long value;
    scores_reset();
    fp=fopen(path,"r");
    if(!fp) return 0;
    if(!fgets(line,sizeof(line),fp) || strcmp(line,"SC1\n"))
        ok=0;
    for(i=0;i<SCORE_COUNT && ok;i++) {
        if(!fgets(line,sizeof(line),fp) ||
           strlen(line)!=15 || line[3]!=' ' || line[14]!='\n') {
            ok=0; break;
        }
        value=0;
        for(j=4;j<14;j++) {
            unsigned digit;
            if(line[j]<'0' || line[j]>'9') { ok=0; break; }
            digit=(unsigned)(line[j]-'0');
            if(value>(4294967295UL-digit)/10UL) { ok=0; break; }
            value=value*10UL+digit;
        }
        if(!ok || (i>0 && value>scores_table[i-1].score))
            { ok=0; break; }
        line[3]=0;
        if((value && !valid_name(line)) ||
           (!value && strcmp(line,"---"))) {
            ok=0; break;
        }
        strcpy(scores_table[i].initials,line);
        scores_table[i].score=value;
    }
    if(ok && fgetc(fp)!=EOF) ok=0; /* Reject trailing garbage. */
    if(fclose(fp)) ok=0;
    if(!ok) scores_reset();
    return ok;
}

/* Only called after a finished qualifying game; never from physics,
 * an interrupt handler, the 18 Hz frame renderer or the /smoke path. */
int scores_save(const char *path)
{
    FILE *fp=fopen(path,"w");
    int i,ok=1;
    if(!fp) return 0;
    if(fputs("SC1\n",fp)==EOF) ok=0;
    for(i=0;i<SCORE_COUNT;i++)
        if(fprintf(fp,"%s %010lu\n",scores_table[i].initials,
                   scores_table[i].score)<0) ok=0;
    if(fclose(fp)) ok=0;
    return ok;
}
