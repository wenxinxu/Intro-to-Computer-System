/**
 * File: mapreduce-mapper.cc
 * -------------------------
 * Presents the implementation of the MapReduceMapper class,
 * which is charged with the responsibility of pressing through
 * a supplied input file through the provided executable and then
 * splaying that output into a large number of intermediate files
 * such that all keys that hash to the same value appear in the same
 * intermediate.
 */

#include "mapreduce-mapper.h"
#include "mr-names.h"
#include "string-utils.h"
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "ostreamlock.h"
using namespace std;

MapReduceMapper::MapReduceMapper(const string& serverHost, unsigned short serverPort,
                                 const string& cwd, const string& executable,
                                 const string& outputPath, size_t splitValue) :
  MapReduceWorker(serverHost, serverPort, cwd, executable, outputPath),splitValue(splitValue){}

void MapReduceMapper::map() const {
  while (true) {
    string name;
    if (!requestInput(name)) break;
    alertServerOfProgress("About to process \"" + name + "\".");
    string base = extractBase(name);
    string output = outputPath + "/" + changeExtension(base, "input", "mapped");
    bool success = processInput(name, output);
    //notifyServer(name, success);

    ifstream in(output);
    string key;
    int val;
    while(in >> key >> val) {
      size_t hashKey = hash<string>()(key) % splitValue;
      string padding = numberToString(hashKey) + ".mapped";
      string newout = outputPath + "/" + changeExtension(base, "input", padding);
      ofstream out;
      out.open(newout,std::ios_base::app);
      out<<key<<" "<<val<<endl;
      out.close();
    }
    in.close();
    notifyServer(name, success);
    remove(output.c_str());
  }

    alertServerOfProgress("Server says no more input chunks, so shutting down.");
}
