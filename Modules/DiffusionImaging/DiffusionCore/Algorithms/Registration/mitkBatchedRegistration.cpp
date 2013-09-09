#include "mitkBatchedRegistration.h"
#include "mitkPyramidImageRegistrationMethod.h"
#include "mitkDiffusionImage.h"

#include <mitkRotationOperation.h>
#include <mitkDiffusionImageCorrectionFilter.h>
#include "itkB0ImageExtractionImageFilter.h"

#include <itkScalableAffineTransform.h>
#include <vnl/vnl_inverse.h>
// VTK
#include <vtkTransform.h>

// DEBUG
#include <mitkIOUtil.h>


mitk::BatchedRegistration::BatchedRegistration() :
  m_RegisteredImagesValid(false)
{
}

void mitk::BatchedRegistration::SetFixedImage(mitk::Image::Pointer& fixedImage)
{
  m_FixedImage = fixedImage;
}

void mitk::BatchedRegistration::SetMovingReferenceImage(Image::Pointer &movingImage)
{
  m_MovingReference = movingImage;
  m_RegisteredImagesValid = false;
}

void mitk::BatchedRegistration::SetBatch(std::vector<mitk::Image::Pointer> imageBatch)
{
  m_ImageBatch.clear();
  m_ImageBatch = imageBatch;
}

std::vector<mitk::Image::Pointer> mitk::BatchedRegistration::GetRegisteredImages()
{
  if (!m_RegisteredImagesValid)
  {
    m_RegisteredImages.clear();
    // First transform moving reference image
    TransformType transf = GetTransformation(m_FixedImage, m_MovingReference);
    // store it as first element in vector
    ApplyTransformationToImage(m_MovingReference,transf);
    m_RegisteredImages.push_back(m_MovingReference);
    // apply transformation to whole batch
    std::vector<mitk::Image::Pointer>::const_iterator itEnd = m_ImageBatch.end();
    for (std::vector<mitk::Image::Pointer>::iterator it = m_ImageBatch.begin(); it != itEnd; ++it)
    {
      ApplyTransformationToImage(*it,transf);
      m_RegisteredImages.push_back(*it);
    }
  }
  return m_RegisteredImages;
}

void mitk::BatchedRegistration::ApplyTransformationToImage(mitk::Image::Pointer &img, const mitk::BatchedRegistration::TransformType &transformation) const
{
  typedef mitk::DiffusionImage<short> DiffusionImageType;

  vtkMatrix4x4* transformationMatrix = vtkMatrix4x4::New();
  vnl_matrix_fixed<double,3,3> vnlRotationMatrix;
  for (int i = 0; i < 4;++i)
    for (int j = 0; j < 4;++j)
    {
      transformationMatrix->Element[i][j] = transformation.get(i,j);
      if (i < 3 && j < 3)
        vnlRotationMatrix.set(i,j,transformation.get(i,j));
    }

  img->GetGeometry()->Compose( transformationMatrix);

  if (dynamic_cast<DiffusionImageType*> (img.GetPointer()) != NULL)
  {
    DiffusionImageType::Pointer diffImages =
        dynamic_cast<DiffusionImageType*>(img.GetPointer());

    // Changes to default geometry does not affect the presentation
    // of Diffusion Images. (No idea why)
    // Workaround:
    // Extracting a B0-image
    // Casting B0 to an MITK image, so we have an image with
    //    the same dimensions and space
    // Apply the transformation to this image
    // Cast back to ITK image
    // Copy Transformation informations from B0-ITK-Image to
    //    diffusion vector image.
    itk::B0ImageExtractionImageFilter<short,short >::Pointer b0Extraction =
        itk::B0ImageExtractionImageFilter<short,short>::New();
    b0Extraction->SetInput(diffImages->GetVectorImage());
    b0Extraction->SetDirections(diffImages->GetDirections());
    b0Extraction->Update();
    mitk::Image::Pointer tmp = mitk::Image::New();
    tmp->InitializeByItk(b0Extraction->GetOutput());
    tmp->SetVolume(b0Extraction->GetOutput()->GetBufferPointer());

    tmp->GetGeometry()->Compose(transformationMatrix);
    itk::Image<short, 3>::Pointer itkTmp;
    mitk::CastToItkImage<itk::Image<short, 3> >(tmp, itkTmp);

    diffImages->GetVectorImage()->SetOrigin(itkTmp->GetOrigin());
    diffImages->GetVectorImage()->SetDirection(itkTmp->GetDirection());
    diffImages->GetVectorImage()->SetSpacing(itkTmp->GetSpacing());

    mitk::DiffusionImageCorrectionFilter<short>::Pointer correctionFilter =
        mitk::DiffusionImageCorrectionFilter<short>::New();

    // For Diff. Images: Need to rotate the gradients
    correctionFilter->SetImage(diffImages);
    correctionFilter->CorrectDirections(vnlRotationMatrix);

  }
}

mitk::BatchedRegistration::TransformType mitk::BatchedRegistration::GetTransformation(mitk::Image::Pointer fixedImage, mitk::Image::Pointer movingImage, mitk::Image::Pointer mask)
{
  // Handle case that fixed or moving image is a DWI image
  mitk::DiffusionImage<short>* fixedDwi = dynamic_cast<mitk::DiffusionImage<short>*> (fixedImage.GetPointer());
  mitk::DiffusionImage<short>* movingDwi = dynamic_cast<mitk::DiffusionImage<short>*> (movingImage.GetPointer());
  itk::B0ImageExtractionImageFilter<short,short >::Pointer b0Extraction = itk::B0ImageExtractionImageFilter<short,short>::New();
  if (fixedDwi != NULL)
  {
    b0Extraction->SetInput(fixedDwi->GetVectorImage());
    b0Extraction->SetDirections(fixedDwi->GetDirections());
    b0Extraction->Update();
    mitk::Image::Pointer tmp = mitk::Image::New();
    tmp->InitializeByItk(b0Extraction->GetOutput());
    tmp->SetVolume(b0Extraction->GetOutput()->GetBufferPointer());
    fixedImage = tmp;
  }
  if (movingDwi != NULL)
  {
    b0Extraction->SetInput(movingDwi->GetVectorImage());
    b0Extraction->SetDirections(movingDwi->GetDirections());
    b0Extraction->Update();
    mitk::Image::Pointer tmp = mitk::Image::New();
    tmp->InitializeByItk(b0Extraction->GetOutput());
    tmp->SetVolume(b0Extraction->GetOutput()->GetBufferPointer());
    movingImage = tmp;
  }
  // Start registration
  mitk::PyramidImageRegistrationMethod::Pointer registrationMethod = mitk::PyramidImageRegistrationMethod::New();
  registrationMethod->SetFixedImage( fixedImage );

  if (mask.IsNotNull())
  {
    registrationMethod->SetFixedImageMask(mask);
    registrationMethod->SetUseFixedImageMask(true);
  }
  else
  {
    registrationMethod->SetUseFixedImageMask(false);
  }

  registrationMethod->SetTransformToRigid();
  registrationMethod->SetCrossModalityOn();

  registrationMethod->SetMovingImage(movingImage);
  registrationMethod->Update();

  TransformType transformation;
  mitk::PyramidImageRegistrationMethod::TransformMatrixType rotationMatrix;
  rotationMatrix = registrationMethod->GetLastRotationMatrix().transpose();
  double param[6];
  registrationMethod->GetParameters(param); // first three: euler angles, last three translation
  for (unsigned int i = 0; i < 3; i++)
  {
    for (unsigned int j = 0; j < 3; ++j)
    {
      double value = rotationMatrix.get(i,j);
      transformation.set(i,j, value);
    }
  }
  transformation.set(0,3,-param[3]);
  transformation.set(1,3,-param[4]);
  transformation.set(2,3,-param[5]);
  transformation.set(3,3,1);
  return transformation;
}
