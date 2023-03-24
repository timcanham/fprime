
    
#include "Tester.hpp"
#include <Os/File.hpp>
#include <Os/FileSystem.hpp>
#include <Os/MicroFs/MicroFs.hpp>
#include <Fw/Types/MallocAllocator.hpp>
#include <Fw/Test/UnitTest.hpp>



  // ------------------------------------------------------------------------------------------------------
  // Rule:  InitFileSystem
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::InitFileSystem::InitFileSystem(U32 numBins, U32 fileSize, U32 numFiles) :
        STest::Rule<Os::Tester>("InitFileSystem")
  {
    this->numBins = numBins;
    this->fileSize = fileSize;
    this->numFiles = numFiles;
  }


  bool Os::Tester::InitFileSystem::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      return true;
  }

  
  void Os::Tester::InitFileSystem::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s \n", this->name);

    ASSERT_LE(this->numBins, Os::MAX_MICROFS_BINS);

    state.testCfg.numBins = this->numBins;

    for (U16 i=0; i < this->numBins; i++)
    {
        state.testCfg.bins[i].fileSize = this->fileSize;
        state.testCfg.bins[i].numFiles = this->numFiles;
    }
    
    Os::MicroFsInit(state.testCfg, 0, state.alloc);

  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  OpenFile
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::OpenFile::OpenFile(const char *filename) :
        STest::Rule<Os::Tester>("OpenFile")
  {
    this->filename = filename;
  }


  bool Os::Tester::OpenFile::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      this->fileIndex = state.getIndex(this->filename);
      return true;
  }

  
  void Os::Tester::OpenFile::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s %s\n", this->name, this->filename);
    Os::Tester::FileModel *fileModel = &(state.fileModels[this->fileIndex]);

    fileModel->curPtr = 0;
    Os::File::Status stat = fileModel->fileDesc.open(this->filename, Os::File::OPEN_WRITE);
    ASSERT_EQ(Os::File::OP_OK, stat);
  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  Cleanup
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::Cleanup::Cleanup() :
        STest::Rule<Os::Tester>("Cleanup")
  {
  }


  bool Os::Tester::Cleanup::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      return true;
  }

  
  void Os::Tester::Cleanup::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s \n", this->name);

    Os::MicroFsCleanup(0,state.alloc);

  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  WriteData
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::WriteData::WriteData(const char *filename, NATIVE_INT_TYPE size, U8 value) :
        STest::Rule<Os::Tester>("WriteData")
  {
    this->size = size;
    this->value = value;
    this->filename = filename;
    
  }


  bool Os::Tester::WriteData::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
    this->fileIndex = state.getIndex(this->filename);
    return true;
  }

  
  void Os::Tester::WriteData::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s %d bytes\n", this->name, this->size);

    ASSERT_NE(this->fileIndex, -1);

    Os::Tester::FileModel *fileModel = &(state.fileModels[this->fileIndex]);

    ASSERT_LE(fileModel->curPtr + this->size, Tester::FILE_SIZE);
    memset(fileModel->buffOut + fileModel->curPtr, this->value, this->size);
    NATIVE_INT_TYPE retSize = this->size;
    Os::File::Status stat = fileModel->fileDesc.write(fileModel->buffOut + fileModel->curPtr, retSize);

    fileModel->curPtr += this->size;
    ASSERT_EQ(stat, Os::File::OP_OK);
    ASSERT_EQ(retSize, this->size);

  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  ReadData
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::ReadData::ReadData(const char *filename, NATIVE_INT_TYPE size) :
        STest::Rule<Os::Tester>("ReadData")
  {
    this->size = size;
    this->filename = filename;
  }


  bool Os::Tester::ReadData::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      this->fileIndex = state.getIndex(this->filename);
      return true;
  }

  
  void Os::Tester::ReadData::action(
            Os::Tester& state //!< The test state
        ) 
  {
      printf("--> Rule: %s \n", this->name);

      ASSERT_NE(this->fileIndex, -1);

      Os::Tester::FileModel *fileModel = &(state.fileModels[this->fileIndex]);

      BYTE buffIn[state.testCfg.bins[0].fileSize];
      NATIVE_INT_TYPE bufferSize = sizeof(buffIn);
      memset(buffIn,0xA5,sizeof(buffIn));
      ASSERT_LE(this->size, sizeof(buffIn));
      NATIVE_INT_TYPE retSize = this->size;
      Os::File::Status stat = fileModel->fileDesc.read(buffIn, retSize);

      ASSERT_EQ(stat, Os::File::OP_OK);
      ASSERT_EQ(retSize, this->size);

      // Check the returned data
      ASSERT_LE(fileModel->curPtr + this->size, Tester::FILE_SIZE);
      ASSERT_EQ(0,memcmp(buffIn, fileModel->buffOut + fileModel->curPtr, this->size));

  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  ResetFile
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::ResetFile::ResetFile(const char *filename) :
        STest::Rule<Os::Tester>("ResetFile")
  {
    this->filename = filename;
  }


  bool Os::Tester::ResetFile::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      this->fileIndex = state.getIndex(this->filename);
      return true;
  }

  
  void Os::Tester::ResetFile::action(
            Os::Tester& state //!< The test state
        ) 
  {
      printf("--> Rule: %s \n", this->name);

      ASSERT_NE(fileIndex, -1);
      Os::Tester::FileModel *fileModel = &(state.fileModels[this->fileIndex]);


      // seek back to beginning
      ASSERT_EQ(Os::File::OP_OK, fileModel->fileDesc.seek(0));
      fileModel->curPtr = 0;
  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  CloseFile
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::CloseFile::CloseFile(const char *filename) :
        STest::Rule<Os::Tester>("CloseFile")
  {
    this->filename = filename;
  }


  bool Os::Tester::CloseFile::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      this->fileIndex = state.getIndex(this->filename);
      return true;
  }

  
  void Os::Tester::CloseFile::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s %s\n", this->name, this->filename);

    Os::Tester::FileModel *fileModel = &(state.fileModels[this->fileIndex]);


    // close file
    ASSERT_NE(fileIndex, -1);
    fileModel->fileDesc.close();
  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  Listings
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::Listings::Listings(U16 numBins, U16 numFiles) :
        STest::Rule<Os::Tester>("Listings")
  {
    this->numBins = numBins;
    this->numFiles = numFiles;
  }


  bool Os::Tester::Listings::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      return true;
  }

  
  void Os::Tester::Listings::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s \n", this->name);

    Fw::String listDir;
    Fw::String expectedFile;
    Fw::String bins[MAX_BINS];
    Fw::String files[MAX_FILES_PER_BIN];
    U32 totalBins = MAX_BINS + 1; // oversize to check return

    COMMENT("Listing /");
    listDir = "/";

    // get root directory listing
    ASSERT_EQ(Os::FileSystem::OP_OK,
        Os::FileSystem::readDirectory(listDir.toChar(), MAX_BINS, bins, totalBins));
    ASSERT_EQ(this->numBins, totalBins);

    for (U16 binIndex=0;  binIndex < this->numBins; binIndex++)
    {
        printf("%s\n", bins[binIndex].toChar());
        expectedFile.format("/%s%d", MICROFS_BIN_STRING, binIndex);
        ASSERT_EQ(0,strcmp(expectedFile.toChar(), bins[binIndex].toChar()));
    }

    U32 totalFiles = MAX_FILES_PER_BIN + 1; // oversize to check return
    for (U16 binIndex=0; binIndex < this->numBins; binIndex++)
    {
        // get file listing
        ASSERT_EQ(Os::FileSystem::OP_OK,
            Os::FileSystem::readDirectory(bins[binIndex].toChar(), this->numFiles, files, totalFiles));
        ASSERT_EQ(this->numFiles, totalFiles);
        COMMENT(bins[binIndex].toChar());
        for (U16 fileIndex=0; fileIndex < this->numFiles; fileIndex++)
        {
            printf("%s\n", files[fileIndex].toChar());
            expectedFile.format("/%s%d/%s%d", MICROFS_BIN_STRING, binIndex, MICROFS_FILE_STRING, fileIndex);
            ASSERT_EQ(0,strcmp(expectedFile.toChar(), files[fileIndex].toChar()));
        }
    }
  }

  // ------------------------------------------------------------------------------------------------------
  // Rule:  FreeSpace
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::FreeSpace::FreeSpace() :
        STest::Rule<Os::Tester>("FreeSpace")
  {
  }


  bool Os::Tester::FreeSpace::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      return true;
  }

  
  void Os::Tester::FreeSpace::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s \n", this->name);

    FwSizeType total;
    FwSizeType free;

    FileSystem::Status stat = FileSystem::getFreeSpace("", total, free);
    ASSERT_EQ(state.m_expFreeBytes,free);
    ASSERT_EQ(state.m_expTotalBytes,total);

  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  OpenNoPerm
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::OpenNoPerm::OpenNoPerm(const char *filename) :
        STest::Rule<Os::Tester>("OpenNoPerm")
  {
    this->filename = filename;
  }


  bool Os::Tester::OpenNoPerm::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      this->fileIndex = state.getIndex(this->filename);
      return true;
  }

  
  void Os::Tester::OpenNoPerm::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s %s\n", this->name, this->filename);
    Os::Tester::FileModel *fileModel = &(state.fileModels[this->fileIndex]);


    ASSERT_NE(fileIndex, -1);
    fileModel->curPtr = 0;
    Os::File::Status stat = fileModel->fileDesc.open(this->filename, Os::File::OPEN_WRITE);
    ASSERT_EQ(Os::File::NO_PERMISSION, stat);
  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  CheckFileSize
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::CheckFileSize::CheckFileSize(const char *filename, FwSizeType size) :
        STest::Rule<Os::Tester>("CheckFileSize")
  {
    this->filename = filename;
    this->size = size;
  }


  bool Os::Tester::CheckFileSize::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      this->fileIndex = state.getIndex(this->filename);
      return true;
  }

  
  void Os::Tester::CheckFileSize::action(
            Os::Tester& state //!< The test state
        ) 
  {
      printf("--> Rule: %s %s, size = %d\n", this->name, this->filename, this->size);
      FwSizeType actualSize = this->size;
      FileSystem::Status stat = FileSystem::getFileSize(this->filename, actualSize);
      ASSERT_EQ(FileSystem::OP_OK, stat);
      ASSERT_EQ(actualSize, this->size);
  }


    


  // ------------------------------------------------------------------------------------------------------
  // Rule:  OpenRead
  //
  // ------------------------------------------------------------------------------------------------------
  
  Os::Tester::OpenRead::OpenRead(const char* filename) :
        STest::Rule<Os::Tester>("OpenRead")
  {
    this->filename = filename;
  }


  bool Os::Tester::OpenRead::precondition(
            const Os::Tester& state //!< The test state
        ) 
  {
      this->fileIndex = state.getIndex(this->filename);
      return true;
  }

  
  void Os::Tester::OpenRead::action(
            Os::Tester& state //!< The test state
        ) 
  {
    printf("--> Rule: %s %s\n", this->name, this->filename);

    ASSERT_NE(fileIndex, -1);

    Os::Tester::FileModel *fileModel = &(state.fileModels[this->fileIndex]);

    Os::File::Status stat = fileModel->fileDesc.open(this->filename, Os::File::OPEN_READ);
    ASSERT_EQ(Os::File::OP_OK, stat);
  }

