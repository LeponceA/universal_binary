#ifndef FILES_ADDER_H__
#define FILES_ADDER_H__

bool WritePackage(FILE* fout, const utf8 parentPath[]);

bool ReproducePackageFiles(FILE* fin, utf8** currentDirName);

#endif
