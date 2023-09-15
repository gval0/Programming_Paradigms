using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include<string.h>

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

// you should be implementing these two methods right here... 
int compare(const void* a, const void* b){
  void* ptr = ((pair<void*, void*>*)a)->first;
  char* chars = (char*) (((pair<void*, void*>*)a)->second);
  int offset = *((int*)b);
  char* cur = (char*)ptr + offset;
  return strcmp(chars, cur);
}

bool imdb::getCredits(const string& player, vector<film>& films) const {
  pair<void*, void*> usefulPair = {(void*)actorFile, (void*)player.c_str()};
  void* finder = bsearch((void*) &usefulPair, (void*)(((int*) actorFile) + 1), *((int*)actorFile), sizeof(int), compare);
  if(!finder) return false;

  int counter = 0;
  char* cur = ((char*) actorFile + *(int*)finder);
  while(*cur != '\0') {
    counter++;
    cur++;
  }
  cur++;
  counter++;
  if(counter % 2 == 1) {
    cur++;
    counter++;
  }
  short moviesN = *(short*)cur;
  counter += 2;
  cur += 2;
  if(counter % 4 != 0){
    cur += 2;
  }

  int* ptr = (int*) cur;
  for(int i = 0; i < moviesN; i++){
    int offset = *(ptr + i);
    cur = (char*)movieFile + offset;
    string title = "";
    while(*cur != '\0'){
      title += *cur;
      cur++;
    }
    film movie;
    movie.title = title;
    cur++;
    movie.year = 1900 + (int)*cur;
    films.push_back(movie);
  }
  return true; 
}

int comp(const void* a, const void* b){
  void* ptr = ((pair<void*, void*>*)a)->first;
  film* neededMovie = (film*) (((pair<void*, void*>*)a)->second);
  int offset = *(int*)b;
  char* cur = (char*)ptr + offset;

  string title = "";
  while(*cur != '\0'){
    title += *cur;
    cur++;
  }
  film movie;
  movie.title = title;
  cur++;
  movie.year = 1900 + (int)*cur;

  if(*neededMovie == movie) return 0;
  else  if(*neededMovie < movie) return -1;
  else return 1;
}

bool imdb::getCast(const film& movie, vector<string>& players) const {
  pair<void*, void*> usefulPair = {(void*)movieFile, (void*)&movie};
  void* finder = bsearch((void*) &usefulPair, (void*)((int*) movieFile + 1), *((int*)movieFile), sizeof(int), comp);
  if(!finder) return false;
  int counter = 0;
  char* cur = ((char*) movieFile + *(int*)finder);
  while(*cur != '\0'){
    counter++;
    cur++;
  }
  cur+=2;
  counter+=2;
  if(counter % 2 == 1) {
    counter++;
    cur++;
  }
  short actorsN = *(short*)cur;
  counter += 2;
  cur += 2;
  if(counter % 4 != 0){
    cur += 2;
  }
  
  int* ptr = (int*) cur;
  for(int i = 0; i < actorsN; i++){
    int offset = *(ptr + i);
    cur = (char*)actorFile + offset;
    string name = "";
    while(*cur != '\0'){
      name += *cur;
      cur++;
    }
    players.push_back(name);
  }
  return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
