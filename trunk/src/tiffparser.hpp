// ***************************************************************** -*- C++ -*-
/*
 * Copyright (C) 2006 Andreas Huggel <ahuggel@gmx.net>
 *
 * This program is part of the Exiv2 distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301 USA.
 */
/*!
  @file    tiffparser.hpp
  @brief   Class TiffParser to parse TIFF data.
  @version $Rev$
  @author  Andreas Huggel (ahu)
           <a href="mailto:ahuggel@gmx.net">ahuggel@gmx.net</a>
  @date    15-Mar-06, ahu: created
 */
#ifndef TIFFPARSER_HPP_
#define TIFFPARSER_HPP_

// *****************************************************************************
// included header files
#include "image.hpp"
#include "exif.hpp"
#include "iptc.hpp"
#include "types.hpp"
#include "error.hpp"

// + standard includes
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cassert>

// *****************************************************************************
// namespace extensions
namespace Exiv2 {

// *****************************************************************************
// class declarations

    struct TiffStructure;
    class TiffDirectory;
    class TiffEntryBase;
    class TiffEntry;
    class TiffSubIfd;

// *****************************************************************************
// type definitions

// *****************************************************************************
// class definitions

    /*!
      Known TIFF groups

      Todo: what exactly are these and where should they go?
      Are they going to be mapped to the second part of an Exif key or are they
      the second part of the key?
    */
    namespace Group {
        const uint16_t none      =   0; //!< Dummy group
        const uint16_t ifd0      =   1; //!< Exif IFD0
        const uint16_t ifd1      =   2; //!< Thumbnail IFD
        const uint16_t exif      =   3; //!< Exif IFD
        const uint16_t gps       =   4; //!< GPS IFD
        const uint16_t iop       =   5; //!< Interoperability IFD
        const uint16_t makernote = 256; //!< Makernote
        const uint16_t canonmn   = 257; //!< Canon makernote
    }

    /*!
      Special TIFF tags for the use in TIFF structures only

      Todo: Same Q as above...
    */
    namespace Tag {
        const uint32_t none = 0x10000; //!< Dummy tag
        const uint32_t root = 0x20000; //!< Special tag: root IFD
        const uint32_t next = 0x30000; //!< Special tag: next IFD
    }

    /*!
      @brief This class models a TIFF header structure.
     */
    class TiffHeade2 {
    public:
        //! @name Creators
        //@{
        //! Default constructor
        TiffHeade2()
            : byteOrder_ (littleEndian),
              offset_    (0x00000008)
            {}
        //@}

        //! @name Manipulators
        //@{
        /*!
          @brief Read the TIFF header from a data buffer. Return false if the
                 data buffer does not contain a TIFF header, else true.

          @param pData Pointer to the data buffer.
          @param size  Number of bytes in the data buffer.
         */
        bool read(const byte* pData, uint32_t size);
        //@}

        //! @name Accessors
        //@{
        /*!
          @brief Write the TIFF header to the binary image \em blob.
                 This method appends to the blob.

          @param blob Binary image to add to.

          @throw Error If the header cannot be written.
         */
        void write(Blob& blob) const;
        /*!
          @brief Print debug info for the TIFF header to \em os.

          @param os Output stream to write to.
          @param prefix Prefix to be written before each line of output.
         */
        void print(std::ostream& os, const std::string& prefix ="") const;
        //! Return the byte order (little or big endian).
        ByteOrder byteOrder() const { return byteOrder_; }
        //! Return the offset to the start of the root directory
        uint32_t offset() const { return offset_; }
        //@}

    private:
        // DATA
        ByteOrder             byteOrder_; //!< Applicable byte order
        uint32_t              offset_;    //!< Offset to the start of the root dir

        static const uint16_t tag_;       //!< 42, identifies the buffer as TIFF data

    }; // class TiffHeade2

    /*!
      @brief Abstract base class for TIFF composite vistors (Visitor pattern)

      A concrete visitor class is used as shown in the example below. Accept()
      will invoke the member function corresponding to the concrete type of each
      component in the composite.

      @code
      void visitorExample(Exiv2::TiffComponent* tiffComponent, Exiv2::TiffVisitor& visitor)
      {
          tiffComponent->accept(visitor);
      }
      @endcode
     */
    class TiffVisitor {
    public:
        //! @name Creators
        //@{
        virtual ~TiffVisitor() {}
        //@}

        //! @name Manipulators
        //@{
        //! Operation to perform for a TIFF entry
        virtual void visitEntry(TiffEntry* object) =0;
        //! Operation to perform for a TIFF directory
        virtual void visitDirectory(TiffDirectory* object) =0;
        //! Operation to perform for a TIFF sub-IFD
        virtual void visitSubIfd(TiffSubIfd* object) =0;
        //@}

    }; // class TiffVisitor

    /*!
      @brief TIFF composite visitor to decode metadata from the TIFF tree and
             add it to an Image, which is supplied in the constructor (Visitor
             pattern). Used by TiffParser to decode the metadata from a
             TIFF composite.
     */
    class TiffMetadataDecoder : public TiffVisitor {
    public:
        //! @name Creators
        //@{
        //! Constructor, taking the image to add the metadata to
        TiffMetadataDecoder(Image* pImage) : pImage_(pImage) {}
        //! Virtual destructor
        virtual ~TiffMetadataDecoder() {}
        //@}

        //! @name Manipulators
        //@{
        //! Decode a TIFF entry
        virtual void visitEntry(TiffEntry* object);
        //! Decode a TIFF directory
        virtual void visitDirectory(TiffDirectory* object);
        //! Decode a TIFF sub-IFD
        virtual void visitSubIfd(TiffSubIfd* object);
        //! Decode a standard TIFF entry
        void decodeTiffEntry(const TiffEntryBase* object);
        //@}

    private:
        // DATA
        Image* pImage_; //!< Pointer to the image to which the metadata is added

    }; // class TiffMetadataDecoder

    /*!
      @brief TIFF composite visitor to read the TIFF structure from a block of
             memory and build the composite from it (Visitor pattern). Used by
             TiffParser to read the TIFF data from a block of memory. Uses
             the policy class CreationPolicy for the creation of TIFF components.
     */
    template<typename CreationPolicy>
    class TiffReader : public TiffVisitor, public CreationPolicy {
    public:
        //! @name Creators
        //@{
        /*!
          @brief Constructor. The data buffer and table describing the TIFF
                           structure of the data are set in the constructor.
          @param pData     Pointer to the data buffer, starting with a TIFF header.
          @param size      Number of bytes in the data buffer.
          @param byteOrder Applicable byte order (little or big endian).
         */
        TiffReader(const byte* pData,
                   uint32_t    size,
                   ByteOrder   byteOrder);

        //! Virtual destructor
        virtual ~TiffReader() {}
        //@}

        //! @name Manipulators
        //@{
        //! Read a TIFF entry from the data buffer
        virtual void visitEntry(TiffEntry* object);
        //! Read a TIFF directory from the data buffer
        virtual void visitDirectory(TiffDirectory* object);
        //! Read a TIFF sub-IFD from the data buffer
        virtual void visitSubIfd(TiffSubIfd* object);
        //! Read a standard TIFF entry from the data buffer
        void readTiffEntry(TiffEntryBase* object);
        //@}

    private:
        // DATA
        const byte* pData_;                     //!< Pointer to the memory buffer
        const uint32_t size_;                   //!< Size of the buffer
        const byte* pLast_;                     //!< Pointer to the last byte
        const ByteOrder byteOrder_;             //!< Byteorder for the image

    }; // class TiffReader

    /*!
      @brief Interface class for components of a TIFF directory hierarchy
             (Composite pattern).  Both TIFF directories as well as entries
             implement this interface.  A component can be un iquely identified
             by a tag, group tupel.  This class is implemented as a NVI
             (Non-Virtual Interface) and it has an interface for visitors (Visitor
             pattern).
     */
    class TiffComponent {
    public:
        //! TiffComponent auto_ptr type
        typedef std::auto_ptr<TiffComponent> AutoPtr;
        //! Container type to hold all metadata
        typedef std::vector<TiffComponent*> Components;

        //! @name Creators
        //@{
        //! Constructor
        TiffComponent(uint16_t tag, uint16_t group)
            : tag_(tag), group_(group), pData_(0) {}

        //! Virtual destructor.
        virtual ~TiffComponent() {}
        //@}

        //! @name Manipulators
        //@{
        //! Add a child to the component. Default is to do nothing.
        void addChild(AutoPtr tiffComponent);
        //! Add a "next" component to the component. Default is to do nothing.
        void addNext(AutoPtr tiffComponent);
        /*!
          @brief Interface to accept visitors (Visitor pattern).

          @param visitor The visitor.
         */
        void accept(TiffVisitor& visitor);
        /*!
          @brief Set a pointer to the start of the binary representation of the
                 component in a memory buffer. The buffer must be allocated and
                 freed outside of this class.
         */
        void setStart(const byte* pData) { pData_ = const_cast<byte*>(pData); }
        //@}

        //! @name Accessors
        //@{
        //! Return the tag of this entry.
        uint16_t tag()                        const { return tag_; }
        //! Return the group id of this component
        uint16_t group()                      const { return group_; }
        //! Return the group name of this component
        std::string groupName() const;
        //! Return a pointer to the start of the binary representation of the component
        const byte* start()                   const { return pData_; }
        /*!
          @brief Print debug info about a component to \em os.

          @param os Output stream to write to
          @param byteOrder Byte order
          @param prefix Prefix to be written before each line of output
         */
        void print(std::ostream& os,
                   ByteOrder byteOrder,
                   const std::string& prefix ="") const;
        //@}

    protected:
        //! @name Manipulators
        //@{
        //! Implements addChild().
        virtual void doAddChild(AutoPtr tiffComponent) {}

        //! Implements addNext().
        virtual void doAddNext(AutoPtr tiffComponent) {}

        //! Implements accept()
        virtual void doAccept(TiffVisitor& visitor) =0;
        //@}

        //! @name Accessors
        //@{
        //! Implements print()
        virtual void doPrint(std::ostream&      os,
                             ByteOrder          byteOrder,
                             const std::string& prefix) const =0;
        //@}

    private:
        // DATA
        uint16_t tag_;      //!< Tag that identifies the component
        uint16_t group_;    //!< Group id for this component
        /*!
          Pointer to the start of the binary representation of the component in
          a memory buffer. The buffer is allocated and freed outside of this class.
        */
        byte*    pData_;

    }; // class TiffComponent

    /*!
      @brief This abstract base class provides the common functionality of an
             IFD directory entry and defines an extended interface for derived
             concrete entries, which allows access to the attributes of the
             entry.
     */
    class TiffEntryBase : public TiffComponent {
        template<typename CreationPolicy>
        friend void TiffReader<CreationPolicy>::readTiffEntry(TiffEntryBase* object);
    public:
        //! @name Creators
        //@{
        //! Default constructor
        TiffEntryBase(uint16_t tag, uint16_t group)
            : TiffComponent(tag, group),
              type_(0), count_(0), offset_(0),
              size_(0), pData_(0), isAllocated_(false), pValue_(0) {}
        //! Virtual destructor.
        virtual ~TiffEntryBase();
        //@}

        //! @name Accessors
        //@{
        //! Return the Exiv2 type which corresponds to the field type.
        TypeId   typeId()        const { return TypeId(type_); }
        //! Return the number of components in this entry.
        uint32_t count()         const { return count_; }
        //! Return the offset relative to the start of the TIFF header.
        uint32_t offset()        const { return offset_; }
        //! Return the size of this component in bytes
        uint32_t size()          const { return size_; }
        //! Return a pointer to the data area of this component
        const byte* pData()      const { return pData_; }
        //! Return a pointer to the converted value of this component
        const Value* pValue()    const { return pValue_; }

        //! Print base entry
        void printEntry(std::ostream&      os,
                        ByteOrder          byteOrder,
                        const std::string& prefix) const;
        //@}

    private:
        // DATA
        uint16_t type_;     //!< Field Type
        uint32_t count_;    //!< The number of values of the indicated Type
        uint32_t offset_;   //!< Offset to data area from start of TIFF header
        /*!
          Size of the data buffer holding the value in bytes, there is no
          minimum size.
         */
        uint32_t size_;
        const byte* pData_; //!< Pointer to the data area
        bool     isAllocated_; //!< True if this entry owns the value data
        Value*   pValue_;   //!< Converted data value

    }; // class TiffEntryBase

    /*!
      @brief A standard TIFF IFD entry.
     */
    class TiffEntry : public TiffEntryBase {
    public:
        //! @name Creators
        //@{
        //! Constructor
        TiffEntry(uint16_t tag, uint16_t group) : TiffEntryBase(tag, group) {}
        //! Virtual destructor.
        virtual ~TiffEntry() {}
        //@}

    private:
        //! @name Manipulators
        //@{
        virtual void doAccept(TiffVisitor& visitor);
        //@}

        //! @name Accessors
        //@{
        virtual void doPrint(std::ostream&      os,
                             ByteOrder          byteOrder,
                             const std::string& prefix) const;
        //@}

    }; // class TiffEntry

    /*!
      @brief This class models a TIFF directory (%Ifd). It is a composite
             component of the TIFF tree.
     */
    class TiffDirectory : public TiffComponent {
    public:
        //! @name Creators
        //@{
        //! Default constructor
        TiffDirectory(uint16_t tag, uint16_t group)
            : TiffComponent(tag, group), pNext_(0) {}
        //! Virtual destructor
        virtual ~TiffDirectory();
        //@}

    private:
        //! @name Manipulators
        //@{
        virtual void doAddChild(TiffComponent::AutoPtr tiffComponent);

        virtual void doAddNext(TiffComponent::AutoPtr tiffComponent);

        virtual void doAccept(TiffVisitor& visitor);
        //@}

        //! @name Accessors
        //@{
        virtual void doPrint(std::ostream&      os,
                             ByteOrder          byteOrder,
                             const std::string& prefix) const;
        //@}

    private:
        // DATA
        Components components_; //!< List of components in this directory
        TiffComponent* pNext_;  //!< Pointer to the next IFD

    }; // class TiffDirectory

    /*!
      @brief This class models a TIFF sub-directory (sub-IFD). A sub-IFD
             is an entry with a value that is a pointer to an IFD
             structure and contains this IFD. The TIFF standard defines
             some important tags to be sub-IFDs, including the %Exif and
             GPS tags.
     */
    class TiffSubIfd : public TiffEntryBase {
        template<typename CreationPolicy>
        friend void TiffReader<CreationPolicy>::visitSubIfd(TiffSubIfd* object);
    public:
        //! @name Creators
        //@{
        //! Default constructor
        TiffSubIfd(uint16_t tag, uint16_t group, uint16_t newGroup)
            : TiffEntryBase(tag, group), ifd_(tag, newGroup) {}
        //! Virtual destructor
        virtual ~TiffSubIfd() {}
        //@}

    private:
        //! @name Manipulators
        //@{
        virtual void doAddChild(TiffComponent::AutoPtr tiffComponent);

        virtual void doAddNext(TiffComponent::AutoPtr tiffComponent);

        virtual void doAccept(TiffVisitor& visitor);
        //@}

        //! @name Accessors
        //@{
        virtual void doPrint(std::ostream&      os,
                             ByteOrder          byteOrder,
                             const std::string& prefix) const;
        //@}

    private:
        // DATA
        TiffDirectory ifd_; //!< The subdirectory

    }; // class TiffSubIfd

    /*!
      Type for a function pointer for functions to create TIFF components.
      Todo: This may eventually need to also have access to the image or parse tree
      in order to make decisions based on the value of other tags.
     */
    typedef TiffComponent::AutoPtr (*NewTiffCompFct)(const TiffStructure* ts);

    /*!
      @brief Data structure used as a row (element) of a table (array)
             describing the TIFF structure of an image format for reading and
             writing.  Different tables can be used to support different TIFF
             based image formats.
     */
    struct TiffStructure {
        //! Return the tag corresponding to the extended tag
        uint16_t tag() const { return static_cast<uint16_t>(extendedTag_ & 0xffff); }

        uint32_t       extendedTag_;    //!< Tag (32 bit so that it can contain special tags)
        uint16_t       group_;          //!< Group that contains the tag
        NewTiffCompFct newTiffCompFct_; //!< Function to create the correct TIFF component
        uint16_t       newGroup_;       //!< Group of the newly created component

        struct Key;
        //! Comparison operator to compare a TiffStructure with a TiffStructure::Key
        bool operator==(const Key& key) const;
    };

    //! Search key for TIFF structure.
    struct TiffStructure::Key {
        //! Constructor
        Key(uint32_t e, uint16_t g) : e_(e), g_(g) {}
        uint32_t e_;                    //!< Extended tag
        uint16_t g_;                    //!< %Group
    };

    /*!
      @brief TIFF component factory for standard TIFF components. This class is
             meant to be used as a policy class.
     */
    class TiffCreator {
    public:
        /*!
          @brief Create the TiffComponent for TIFF entry \em extendedTag and 
                 \em group based on the embedded lookup table.

          If a tag and group combination is not found in the table, a TiffEntry
          is created.  If the pointer that is returned is 0, then the TIFF entry
          should be ignored.
        */
        static TiffComponent::AutoPtr create(uint32_t extendedTag,
                                             uint16_t group);
    protected:
        //! Prevent destruction
        ~TiffCreator() {}
    private:
        static const TiffStructure tiffStructure_[]; //<! TIFF structure
    }; // class TiffCreator

    /*!
      @brief Stateless parser class for data in TIFF format. Images use this
             class to decode and encode TIFF-based data. Uses class 
             CreationPolicy for the creation of TIFF components.
     */
    template<typename CreationPolicy>
    class TiffParser : public CreationPolicy {
    public:
        /*!
          @brief Decode TIFF metadata from a data buffer \em pData of length
                 \em size into \em pImage.

          This is the entry point to access image data in TIFF format. The
          parser uses classes TiffHeade2 and the TiffComponent and TiffVisitor
          hierarchies.

          @param pImage         Pointer to the image to hold the metadata
          @param pData          Pointer to the data buffer. Must point to data
                                in TIFF format; no checks are performed.
          @param size           Length of the data buffer.
        */
        static void decode(      Image*         pImage,
                           const byte*          pData,
                                 uint32_t       size);
    }; // class TiffParser

// *****************************************************************************
// template, inline and free functions

    //! Function to create and initialize a new TIFF directory
    TiffComponent::AutoPtr newTiffDirectory(const TiffStructure* ts);

    //! Function to create and initialize a new TIFF sub-directory
    TiffComponent::AutoPtr newTiffSubIfd(const TiffStructure* ts);

    template<typename CreationPolicy>
    void TiffParser<CreationPolicy>::decode(Image* pImage,
                                            const byte* pData,
                                            uint32_t size)
    {
        assert(pImage != 0);
        assert(pData != 0);

        TiffHeade2 tiffHeader;
        if (!tiffHeader.read(pData, size) || tiffHeader.offset() >= size) {
            throw Error(3, "TIFF");
        }
        TiffComponent::AutoPtr rootDir = CreationPolicy::create(Tag::root, 
                                                                Group::none);
        if (0 == rootDir.get()) return;
        rootDir->setStart(pData + tiffHeader.offset());

        TiffReader<CreationPolicy> reader(pData,
                                          size,
                                          tiffHeader.byteOrder());
        rootDir->accept(reader);

        TiffMetadataDecoder decoder(pImage);
        rootDir->accept(decoder);

    } // TiffParser::decode

    template<typename CreationPolicy>
    TiffReader<CreationPolicy>::TiffReader(const byte* pData,
                                           uint32_t    size,
                                           ByteOrder   byteOrder)
        : pData_(pData),
          size_(size),
          pLast_(pData + size - 1),
          byteOrder_(byteOrder)
    {
        assert(pData);
        assert(size > 0);
    } // TiffReader::TiffReader

    template<typename CreationPolicy>
    void TiffReader<CreationPolicy>::visitEntry(TiffEntry* object)
    {
        readTiffEntry(object);
    }

    template<typename CreationPolicy>
    void TiffReader<CreationPolicy>::visitDirectory(TiffDirectory* object)
    {
        assert(object != 0);

        byte* p = const_cast<byte*>(object->start());
        assert(p >= pData_);

        if (p + 2 > pLast_) {
#ifndef SUPPRESS_WARNINGS
            std::cerr << "Error: "
                      << "Directory " << object->groupName() << ": "
                      << " IFD exceeds data buffer, cannot read entry count.\n";
#endif
            return;
        }
        const uint16_t n = getUShort(p, byteOrder_);
        p += 2;
        for (uint16_t i = 0; i < n; ++i) {
            if (p + 12 > pLast_) {
#ifndef SUPPRESS_WARNINGS
                std::cerr << "Error: "
                          << "Directory " << object->groupName() << ": "
                          << " IFD entry " << i
                          << " lies outside of the data buffer.\n";
#endif
                return;
            }
            uint16_t tag = getUShort(p, byteOrder_);
            TiffComponent::AutoPtr tc = CreationPolicy::create(tag, object->group());
            tc->setStart(p);
            object->addChild(tc);
            p += 12;
        }

        if (p + 4 > pLast_) {
#ifndef SUPPRESS_WARNINGS
                std::cerr << "Error: "
                          << "Directory " << object->groupName() << ": "
                          << " IFD exceeds data buffer, cannot read next pointer.\n";
#endif
                return;
        }
        uint32_t next = getLong(p, byteOrder_);
        if (next) {
            TiffComponent::AutoPtr tc = CreationPolicy::create(Tag::next, object->group());
            if (next > size_) {
#ifndef SUPPRESS_WARNINGS
                std::cerr << "Error: "
                          << "Directory " << object->groupName() << ": "
                          << " Next pointer is out of bounds.\n";
#endif
                return;
            }
            tc->setStart(pData_ + next);
            object->addNext(tc);
        }

    } // TiffReader::visitDirectory

    template<typename CreationPolicy>
    void TiffReader<CreationPolicy>::visitSubIfd(TiffSubIfd* object)
    {
        assert(object != 0);

        readTiffEntry(object);
        if (object->typeId() == unsignedLong && object->count() >= 1) {
            uint32_t offset = getULong(object->pData(), byteOrder_);
            if (offset > size_) {
#ifndef SUPPRESS_WARNINGS
                std::cerr << "Error: "
                          << "Directory " << object->groupName()
                          << ", entry 0x" << std::setw(4)
                          << std::setfill('0') << std::hex << object->tag()
                          << " Sub-IFD pointer is out of bounds; ignoring it.\n";
#endif
                return;
            }
            object->ifd_.setStart(pData_ + offset);
        }
#ifndef SUPPRESS_WARNINGS
        else {
            std::cerr << "Warning: "
                      << "Directory " << object->groupName()
                      << ", entry 0x" << std::setw(4)
                      << std::setfill('0') << std::hex << object->tag()
                      << " doesn't look like a sub-IFD.";
        }
#endif

    } // TiffReader::visitSubIfd

    template<typename CreationPolicy>
    void TiffReader<CreationPolicy>::readTiffEntry(TiffEntryBase* object)
    {
        assert(object != 0);

        byte* p = const_cast<byte*>(object->start());
        assert(p >= pData_);

        if (p + 12 > pLast_) {
#ifndef SUPPRESS_WARNINGS
            std::cerr << "Error: Entry in directory " << object->groupName()
                      << "requests access to memory beyond the data buffer. "
                      << "Skipping entry.\n";
#endif
            return;
        }
        // Component already has tag
        p += 2;
        object->type_ = getUShort(p, byteOrder_);
        // todo: check type
        p += 2;
        object->count_ = getULong(p, byteOrder_);
        p += 4;
        object->size_ = TypeInfo::typeSize(object->typeId()) * object->count();
        object->offset_ = getULong(p, byteOrder_);
        object->pData_ = p;
        if (object->size() > 4) {
            if (object->offset() >= size_) {
#ifndef SUPPRESS_WARNINGS
                std::cerr << "Error: Offset of "
                          << "directory " << object->groupName() << ", "
                          << " entry 0x" << std::setw(4)
                          << std::setfill('0') << std::hex << object->tag()
                          << " is out of bounds:\n"
                          << "Offset = 0x" << std::setw(8)
                          << std::setfill('0') << std::hex << object->offset()
                          << "; truncating the entry\n";
#endif
                object->size_ = 0;
                object->count_ = 0;
                object->offset_ = 0;
                return;
            }
            object->pData_ = pData_ + object->offset();
            if (object->pData() + object->size() > pLast_) {
#ifndef SUPPRESS_WARNINGS
                std::cerr << "Warning: Upper boundary of data for "
                          << "directory " << object->groupName() << ", "
                          << " entry 0x" << std::setw(4)
                          << std::setfill('0') << std::hex << object->tag()
                          << " is out of bounds:\n"
                          << "Offset = 0x" << std::setw(8)
                          << std::setfill('0') << std::hex << object->offset()
                          << ", size = " << std::dec << object->size()
                          << ", exceeds buffer size by "
                // cast to make MSVC happy
                          << static_cast<uint32_t>(object->pData() + object->size() - pLast_)
                          << " Bytes; adjusting the size\n";
#endif
                object->size_ = size_ - object->offset();
                // todo: adjust count_, make size_ a multiple of typeSize
            }
        }
        Value::AutoPtr v = Value::create(object->typeId());
        if (v.get()) {
            v->read(object->pData(), object->size(), byteOrder_);
            object->pValue_ = v.release();
        }

    } // TiffReader::readTiffEntry

}                                       // namespace Exiv2

#endif                                  // #ifndef TIFFPARSER_HPP_
