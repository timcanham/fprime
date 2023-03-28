

    // ------------------------------------------------------------------------------------------------------
    // Rule:  InitFileSystem
    //
    // ------------------------------------------------------------------------------------------------------
    struct InitFileSystem : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            InitFileSystem(U32 numBins, U32 fileSize, U32 numFiles);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            U32 numBins;
            U32 fileSize;
            U32 numFiles;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  OpenFile
    //
    // ------------------------------------------------------------------------------------------------------
    struct OpenFile : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            OpenFile(const char *filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char *filename; 
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  Cleanup
    //
    // ------------------------------------------------------------------------------------------------------
    struct Cleanup : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            Cleanup();

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  WriteData
    //
    // ------------------------------------------------------------------------------------------------------
    struct WriteData : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            WriteData(const char *filename, NATIVE_INT_TYPE size, U8 value);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            NATIVE_INT_TYPE size;
            U8 value;
            const char* filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  ReadData
    //
    // ------------------------------------------------------------------------------------------------------
    struct ReadData : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            ReadData(const char *filename, NATIVE_INT_TYPE size);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            NATIVE_INT_TYPE size;
            const char *filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  ResetFile
    //
    // ------------------------------------------------------------------------------------------------------
    struct ResetFile : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            ResetFile(const char *filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char *filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  CloseFile
    //
    // ------------------------------------------------------------------------------------------------------
    struct CloseFile : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            CloseFile(const char *filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char *filename;
            Os::Tester::FileModel *fileModel;

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  Listings
    //
    // ------------------------------------------------------------------------------------------------------
    struct Listings : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            Listings(U16 numBins, U16 numFiles);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            U16 numBins;
            U16 numFiles;

    };

    // ------------------------------------------------------------------------------------------------------
    // Rule:  FreeSpace
    //
    // ------------------------------------------------------------------------------------------------------
    struct FreeSpace : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            FreeSpace();

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

    };
    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  OpenNoPerm
    //
    // ------------------------------------------------------------------------------------------------------
    struct OpenNoPerm : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            OpenNoPerm(const char *filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char *filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  CheckFileSize
    //
    // ------------------------------------------------------------------------------------------------------
    struct CheckFileSize : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            CheckFileSize(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char* filename;
            Os::Tester::FileModel *fileModel;

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  OpenRead
    //
    // ------------------------------------------------------------------------------------------------------
    struct OpenRead : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            OpenRead(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char* filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  OpenFileNotExist
    //
    // ------------------------------------------------------------------------------------------------------
    struct OpenFileNotExist : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            OpenFileNotExist(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char* filename;

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  OpenReadEarly
    //
    // ------------------------------------------------------------------------------------------------------
    struct OpenReadEarly : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            OpenReadEarly(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char* filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  OpenCreate
    //
    // ------------------------------------------------------------------------------------------------------
    struct OpenCreate : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            OpenCreate(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );
            const char* filename;
            Os::Tester::FileModel *fileModel;

    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  OpenAppend
    //
    // ------------------------------------------------------------------------------------------------------
    struct OpenAppend : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            OpenAppend(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char* filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  RemoveFile
    //
    // ------------------------------------------------------------------------------------------------------
    struct RemoveFile : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            RemoveFile(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char* filename;
            Os::Tester::FileModel *fileModel;


    };

    



    // ------------------------------------------------------------------------------------------------------
    // Rule:  RemoveBusyFile
    //
    // ------------------------------------------------------------------------------------------------------
    struct RemoveBusyFile : public STest::Rule<Os::Tester> {

            // ----------------------------------------------------------------------
            // Construction
            // ----------------------------------------------------------------------

            //! Constructor
            RemoveBusyFile(const char* filename);

            // ----------------------------------------------------------------------
            // Public member functions
            // ----------------------------------------------------------------------

            //! Precondition
            bool precondition(
                const Os::Tester& state //!< The test state
            );

            //! Action
            void action(
                Os::Tester& state //!< The test state
            );

            const char* filename;
            Os::Tester::FileModel *fileModel;

            

    };

    