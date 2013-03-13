/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


#ifndef _MITK_POINT_SET_SOURCE_H
#define _MITK_POINT_SET_SOURCE_H

#include "mitkBaseProcess.h"
#include "mitkPointSet.h"

namespace mitk
{
/**
 * @brief Superclass of all classes generating point sets (instances of class
 * mitk::PointSet) as output.
 *
 * In itk and vtk the generated result of a ProcessObject is only guaranteed
 * to be up-to-date, when Update() of the ProcessObject or the generated
 * DataObject is called immediately before access of the data stored in the
 * DataObject.
 * @ingroup Process
 */
class MITK_CORE_EXPORT PointSetSource : public BaseProcess
{
public:
    mitkClassMacro( PointSetSource, BaseProcess );

    itkNewMacro( Self );

    typedef PointSet OutputType;

    typedef OutputType::Pointer OutputTypePointer;

    /**
     * Allocates a new output object and returns it. Currently the
     * index idx is not evaluated.
     * @param idx the index of the output for which an object should be created
     * @returns the new object
     */
    virtual itk::DataObject::Pointer MakeOutput ( DataObjectPointerArraySizeType idx );

    /**
     * This is a default implementation to make sure we have something.
     * Once all the subclasses of ProcessObject provide an appopriate
     * MakeOutput(), then ProcessObject::MakeOutput() can be made pure
     * virtual.
     */
    virtual itk::DataObject::Pointer MakeOutput(const DataObjectIdentifierType &name);

    OutputType* GetOutput(const DataObjectIdentifierType & key);
    const OutputType* GetOutput(const DataObjectIdentifierType & key) const;
    OutputType* GetOutput(DataObjectPointerArraySizeType idx);
    const OutputType* GetOutput(DataObjectPointerArraySizeType idx) const;

//    /**
//     * Allows to set the output of the point set source.
//     * @param output the intended output of the point set source
//     */
//    virtual void SetOutput( OutputType* output );

    virtual void GraftOutput(OutputType *output);
    virtual void GraftNthOutput(DataObjectPointerArraySizeType idx, OutputType *output);

protected:

    PointSetSource();

    virtual ~PointSetSource();

};

}
#endif // #define _MITK_POINT_SET_SOURCE_H
