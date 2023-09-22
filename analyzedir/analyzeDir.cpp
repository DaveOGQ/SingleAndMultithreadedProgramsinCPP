//Author:David Oti-George

#include "analyzeDir.h"

#include <cassert>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <sys/resource.h>
#include <sys/time.h>

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <set>

constexpr int MAX_WORD_SIZE = 1024;

// check if path refers to a directory
static bool is_dir(const std::string & path)
{
  struct stat buff;
  if (0 != stat(path.c_str(), &buff)) return false;
  return S_ISDIR(buff.st_mode);
}

/// check if path refers to a file
static bool is_file(const std::string & path)
{
  struct stat buff;
  if (0 != stat(path.c_str(), &buff)) return false;
  return S_ISREG(buff.st_mode);
}

/// check if string ends with another string
static bool ends_with(const std::string & str, const std::string & suffix)
{
  if (str.size() < suffix.size()) return false;
  else
    return 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

//contains the directory count, file count and list of vacant directories for each directory
struct vacantResult {
  long dirCount;
  long fileCount;
  std::vector<std::string> vacantDir;
};

//should return the next word in a file, and "" on EOF
std::string next_word(FILE * file){
  std::string result;
  while(1) {
    int c = fgetc(file);
    if(c == EOF)break;
    c = tolower(c);
    if(isalpha(c)) {
      if(result.size() >= MAX_WORD_SIZE) {
        printf("input exceeded %d word size, aborting...\n", MAX_WORD_SIZE);
        exit(-1);
      }
      result.push_back(c);
    }
    else {
      if(result.size() == 0){
        continue;
      }else{
        break;
      }
    }
  }
  return result;
}

// returns the vacantResult Stuct for the given directory
static vacantResult scan(const std::string & dir, std::vector<std::string> & files)
{
  long dCount = 0; // number of directories discovered so far
  long fCount = 0; //number of files discovered so far
  std::vector<std::string> vacantDirs; //vacant directories found
  //open directory
  auto dirp = opendir( dir.c_str());
  assert(dirp != nullptr);
  for (auto de = readdir(dirp); de != nullptr; de = readdir(dirp)) {
    std::string name = de->d_name;
    if (name == "." or name == "..") continue;
    std::string path = dir + "/" + name;
    //if item is directory is a file
    if (is_file(path)) {
      fCount++; // increase number of files found in this directory by 1
      files.push_back(path); // add this file to the list of files to run statistics on later
      continue; //continue to the next item in the directory
    }
    //if item in directory is another directory
    if(is_dir(path)){
      dCount++; //increase directory count
      auto sub_result = scan(path, files); //get the count of files in the subdirectory and the list of vacant directories within it
      vacantDirs.insert(std::end(vacantDirs), std::begin(sub_result.vacantDir), std::end(sub_result.vacantDir)); 
      // add the list of vacant directories in the subdirectory to the list of vacant directories in the current one
      fCount += sub_result.fileCount;  // add the file count of the subdirectory to the current one
      dCount += sub_result.dirCount;  // add the directory count of the subdirectory to the current one
    }
  }
  closedir(dirp);
  //if there are no files under this directory after scanning for files and directories, return this directory as the only vacant directory
  if (fCount == 0){
    vacantDirs.clear();
    if(dir == "."){
    vacantDirs.push_back(dir);//if the vacant directory is the root directory  return as is
    }else {
     vacantDirs.push_back(dir.substr(2, dir.size()));//if its a subdirectory of the root directory remove "./"
    }
    
  }
  return {dCount, fCount, vacantDirs};
}

//custom comparision fucntion for sorting the Images vector, sorts by size
bool compareBySize( ImageInfo &a,  ImageInfo &b){
  long asize = a.width * a.height;
  long bsize = b.width * b.height;
  return asize > bsize;
}


Results analyzeDir(int n)
{
  Results res;

  //vectort to store files
  std::vector<std::string> Files;

  //set path to current directory
  std::string dir_name = ".";

  //unordered map for word found in txt files
  std::unordered_map<std::string,int> hist;

  //vector to store all images
  std::vector<ImageInfo> images; 

  //scan directory
  vacantResult recurse =  scan(dir_name, Files);

  res.n_dirs = recurse.dirCount + 1; //add the amount of subdirectories + 1 to include root directory
  res.n_files = recurse.fileCount; //add the file count
  res.vacant_dirs.insert(std::end(res.vacant_dirs), std::begin(recurse.vacantDir), std::end(recurse.vacantDir)); //add to the vector of vacant directories

  res.largest_file_path = "";
  res.largest_file_size = -1;
  res.all_files_size = 0;

  long currentFileSize = 0;

  if(Files.size() != 0){
    for(std::string path : Files){
      ImageInfo pic;
      pic.path = path.substr(2,path.size()); //set name of the image 

      //Get image info using identify
      std::string cmd = "identify -format '%w %h' " + path + " 2> /dev/null";
      std::string img_size;
      auto fp = popen(cmd.c_str(), "r");
      assert(fp);
      char buff[PATH_MAX];
      if( fgets(buff, PATH_MAX, fp) != NULL) {
        img_size = buff;
      }
      int status = pclose(fp);

      if( status != 0 or img_size[0] == '0'){
        img_size = "";
      }

      //if its an image img_size will be non-empty and then assign the dimensions to the imageinfo fields, width and length, using sscanf
      if( !img_size.empty()) {
        sscanf(img_size.c_str(), "%ld %ld", &pic.width, &pic.height);
        images.push_back(pic); // add this image to the images array
      }

      //-------------------------------------------------------------------
      struct stat buffer;
      //get size of file
      if (0 != stat(path.c_str(), &buffer)) {
        //if theres an error with getting file size print this message
        printf("    - could not determine size of file\n");
      }else{
        //if gettng the file size is succcesfull
        currentFileSize = long(buffer.st_size);//get the size of file
        res.all_files_size += currentFileSize;//add the size of the file to the total 
        //if the current file size is greater than the largest file so far, make it the largest file
        if (currentFileSize > res.largest_file_size){
          res.largest_file_size = currentFileSize;
          res.largest_file_path = path.substr(2,path.size());
        }
      }

      //--------------------------------------------------------------------
      //getting most common words
      if(ends_with(path.c_str(), ".txt")){
        FILE * file = fopen(path.c_str(), "r");
        
        while(1){
          auto w = next_word(file);

          if(w.size() == 0) break;

          //ignore words with less than 5 letters
          if(w.size() < 5) continue;

          hist[w] ++;
        }
        fclose(file);
      }
      
    }
  }

  if(images.size() > size_t(n)) {
    //sort only up to the nth image
    std::partial_sort(images.begin(), images.begin() + n, images.end(), compareBySize);
    // drop all entries after the first n
    images.resize(n);
  } else {
    //sort all images
    std::sort(images.begin(), images.end(), compareBySize);
  }

  //put the contents of hist into a vector, with count first and the word second, and make count negative to reverse the sort
  std::vector<std::pair<int,std::string>> arr;
  for(auto & h : hist){
    arr.emplace_back(-h.second, h.first);
  }
  
  if(arr.size() > size_t(n)){
    //if arr is greater than n only sort up till the nth pair 

    std::partial_sort(arr.begin(), arr.begin() + n, arr.end());
    // drop all entries after the first n
    arr.resize(n);
  } else {
    //sort every pair
    std::sort(arr.begin(), arr.end());
  }

  for(auto & a : arr){
    //flip the order once more and put contents of arr into the result
    res.most_common_words.push_back({a.second.c_str(), -a.first});
  }

  //add images to the result
  // res.largest_images.insert(std::end(res.largest_images), std::begin(images), std::end(images));
  res.largest_images = images;

  return res;
}