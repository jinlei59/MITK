#include <mitkCollectionStatistic.h>
#include <mitkDataCollection.h>

// DataCollection Stuff
#include <mitkDataCollectionImageIterator.h>
//stl stuff
#include <sstream>

void mitk::CollectionStatistic::SetCollection(mitk::DataCollection::Pointer collection)
{
    m_Collection = collection;
}


mitk::DataCollection::Pointer mitk::CollectionStatistic::GetCollection()
{
    return m_Collection;
}


void mitk::CollectionStatistic::SetClassCount (size_t count)
{
  m_ClassCount = count;
}

size_t mitk::CollectionStatistic::GetClassCount()
{
  return m_ClassCount;
}

void mitk::CollectionStatistic::SetGoldName(std::string name)
{
  m_GroundTruthName = name;
}

std::string mitk::CollectionStatistic::GetGoldName()
{
  return m_GroundTruthName;
}

void mitk::CollectionStatistic::SetTestName(std::string name)
{
  m_TestName = name;
}

std::string mitk::CollectionStatistic::GetTestName()
{
  return m_TestName;
}

int mitk::CollectionStatistic::IsInSameVirtualClass(unsigned char gold, unsigned char test)
{
  int resultClass = -1;
  for (int i = 0; i < m_ConnectionGold.size(); ++i)
  {
    if (m_ConnectionGold[i] == gold && m_ConnectionTest[i] == test)
    {
      resultClass = m_ConnectionClass[i];
      break;
    }
  }
  return resultClass;
}

unsigned char mitk::CollectionStatistic::ToGoldIndex(unsigned char value)
{
  return value;
}
unsigned char mitk::CollectionStatistic::ToTestIndex(unsigned char value)
{
  if (value == 1 || value == 5)
    return 0;
  else
    return 1;
}
bool mitk::CollectionStatistic::Update()
{
  typedef itk::Image<unsigned char, 3> ImageType;
  DataCollectionImageIterator<unsigned char, 3> goldIter(m_Collection, m_GroundTruthName);
  DataCollectionImageIterator<unsigned char, 3> testIter(m_Collection, m_TestName);
  DataCollectionImageIterator<unsigned char, 3> maskIter(m_Collection, m_MaskName);

  int index = 0;

  while (!goldIter.IsAtEnd())
  {
    std::size_t imageIndex = goldIter.GetImageIndex();
    if (m_ImageClassStatistic.size() <= imageIndex)
    {
      MITK_INFO << "New Image: " << goldIter.GetFilePrefix();
      m_ImageNames.push_back(goldIter.GetFilePrefix());
      StatisticData statData;
      m_ImageStatistic.push_back(statData);
      DataVector data;
      for (int i = 0; i < m_ClassCount; ++i)
      {
        StatisticData stat;
        data.push_back(stat);
      }
      m_ImageClassStatistic.push_back(data);
    }

    if (maskIter.GetVoxel() <= 0)
    {
      ++goldIter;
      ++testIter;
      ++maskIter;
      continue;
    }

    ++index;
    unsigned char goldClass = ToGoldIndex(goldIter.GetVoxel());
    unsigned char testClass = ToTestIndex(testIter.GetVoxel());
    if (goldClass == testClass) // True Positive
    {
      m_ImageStatistic[imageIndex].m_TruePositive += 1;
      for (int i = 0; i < m_ClassCount; ++i)
      {
        if (goldClass == i) // For the detected class it is a true positive
        {
          m_ImageClassStatistic[imageIndex][i].m_TruePositive += 1;
        } else // for all other classes than the detected it is a true negative
        {
          m_ImageClassStatistic[imageIndex][i].m_TrueNegative += 1;
        }
      }
    } else // No True Positive
    {
      m_ImageStatistic[imageIndex].m_FalseNegative += 1;
      m_ImageStatistic[imageIndex].m_FalsePositive += 1;
      for (int i = 0; i < m_ClassCount; ++i)
      {
        if (goldClass == i) // For the class in Goldstandard it is a false negative
        {
          m_ImageClassStatistic[imageIndex][i].m_FalseNegative += 1;
        } else if ( testClass == i) // For the test class it is a false positive
        {
          m_ImageClassStatistic[imageIndex][i].m_FalsePositive += 1;
        } else // For all other it is a true negative
        {
          m_ImageClassStatistic[imageIndex][i].m_TrueNegative += 1;
        }
      }
    }

    ++goldIter;
    ++testIter;
    ++maskIter;
  }
  MITK_INFO << "Evaluated " << index << " points";
  return true;
}


void mitk::CollectionStatistic::Print(std::ostream& out, std::ostream& sout, bool withHeader, std::string label)
{
  assert(m_ImageClassStatistic[0].size() == m_ClassCount);
  if (withHeader)
  {
    sout << "Label;ImageName;";
    for (int i = 0; i < m_ImageClassStatistic[0].size(); ++i)
    {
      sout << "DICE-Class-"<< i << ";";
      sout << "Jaccard-Class-"<< i << ";";
      sout << "Sensitivity-Class-"<< i << ";";
      sout << "Specificity-Class-"<< i << ";";
      sout << "TP-Class-"<< i << ";";
      sout << "TN-Class-"<< i << ";";
      sout << "FP-Class-"<< i << ";";
      sout << "FN-Class-"<< i << ";";
    }
    sout << "DICE-MEAN"<< ";";
    sout << "Jaccard-MEAN"<< ";";
    sout << "Sensitivity-MEAN"<< ";";
    sout << "Specificity-MEAN"<< ";";
    sout << "TP-MEAN"<< ";";
    sout << "TN-MEAN"<< ";";
    sout << "FP-MEAN"<< ";";
    sout << "FN-MEAN"<< ";";
    sout << "DICE-WMEAN"<< ";";
    sout << "Jaccard-WMEAN"<< ";";
    sout << "Sensitivity-WMEAN"<< ";";
    sout << "Specificity-WMEAN"<< ";";
    sout << "TP-WMEAN"<< ";";
    sout << "TN-WMEAN"<< ";";
    sout << "FP-WMEAN"<< ";";
    sout << "FN-WMEAN"<< ";";
    sout << "COMPLET-TRUE/FALSE"<< ";";
    sout << "COMPLETE-TRUES"<< ";";
    sout << "COMPLETE_FALSE"<< ";";
    sout << std::endl;
  }
  out << std::setprecision(5);
  for (int i = 0; i < m_ImageClassStatistic.size(); ++i)
  {
    sout << label << ";"<< m_ImageNames[i]<<";";
    StatisticData meanStat;
    StatisticData wMeanStat;
    double pointsSum = 0;

    out << "======================================================== Image " << std::setw(3) << i << " ========================================================" << std::endl;
    out << " Image ID : " << m_ImageNames[i] <<std::endl;

    out << "|--------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|--------------|" << std::endl;
    out << "| Class  |    DICE     |   Jaccard   | Sensitivity | Specificity |     TP      |     TN      |     FP      |      FN      |" << std::endl;
    out << "|--------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|--------------|" << std::endl;


    for (int j =0; j < m_ImageClassStatistic[i].size(); ++j)
    {
      StatisticData stat = m_ImageClassStatistic[i][j];
      stat.m_DICE  = std::max(0.0,(2.0 * stat.m_TruePositive) / (2.0 * stat.m_TruePositive + stat.m_FalseNegative + stat.m_FalsePositive));
      stat.m_Jaccard = std::max(0.0,(1.0 * stat.m_TruePositive) / (1.0 * stat.m_TruePositive + stat.m_FalseNegative + stat.m_FalsePositive));
      stat.m_Sensitivity = std::max(0.0,(1.0 * stat.m_TruePositive) / (stat.m_TruePositive + stat.m_FalseNegative));
      stat.m_Specificity = std::max(0.0,(1.0 * stat.m_TrueNegative)  / ( stat.m_FalsePositive + stat.m_TrueNegative));

      meanStat.m_DICE += std::max(stat.m_DICE,0.0);
      meanStat.m_Jaccard += std::max(stat.m_Jaccard,0.0);
      meanStat.m_Sensitivity += std::max(stat.m_Sensitivity,0.0);
      meanStat.m_Specificity += std::max(stat.m_Specificity,0.0);
      meanStat.m_TruePositive += stat.m_TruePositive;
      meanStat.m_TrueNegative += stat.m_TrueNegative;
      meanStat.m_FalsePositive += stat.m_FalsePositive;
      meanStat.m_FalseNegative += stat.m_FalseNegative;

      double points = stat.m_TruePositive + stat.m_FalseNegative;
      pointsSum += points;

      wMeanStat.m_DICE += std::max(stat.m_DICE,0.0) * points;
      wMeanStat.m_Jaccard += std::max(stat.m_Jaccard,0.0) * points;
      wMeanStat.m_Sensitivity += std::max(stat.m_Sensitivity,0.0) * points;
      wMeanStat.m_Specificity += std::max(stat.m_Specificity,0.0) * points;
      wMeanStat.m_TruePositive += stat.m_TruePositive;
      wMeanStat.m_TrueNegative += stat.m_TrueNegative;
      wMeanStat.m_FalsePositive += stat.m_FalsePositive;
      wMeanStat.m_FalseNegative += stat.m_FalseNegative;

      out<< "|" << std::setw(7) << j << " | ";
      out << std::setw(11) << stat.m_DICE << " | ";
      out << std::setw(11) << stat.m_Jaccard << " | ";
      out << std::setw(11) << stat.m_Sensitivity  << " | ";
      out << std::setw(11) << stat.m_Specificity << " | ";
      out << std::setw(11) << stat.m_TruePositive << " | ";
      out << std::setw(11) << stat.m_TrueNegative<< " | ";
      out << std::setw(11) << stat.m_FalsePositive<< " | ";
      out << std::setw(11) << stat.m_FalseNegative << "  |"<< std::endl;
      sout << stat.m_DICE << ";";
      sout << stat.m_Jaccard << ";";
      sout << stat.m_Sensitivity  << ";";
      sout << stat.m_Specificity << ";";
      sout << stat.m_TruePositive << ";";
      sout << stat.m_TrueNegative<< ";";
      sout << stat.m_FalsePositive<< ";";
      sout << stat.m_FalseNegative << ";";
    }

    meanStat.m_DICE /=  m_ImageClassStatistic[i].size();
    meanStat.m_Jaccard /=  m_ImageClassStatistic[i].size();
    meanStat.m_Sensitivity /=  m_ImageClassStatistic[i].size();
    meanStat.m_Specificity /=  m_ImageClassStatistic[i].size();
    meanStat.m_TruePositive /=  m_ImageClassStatistic[i].size();
    meanStat.m_TrueNegative /=  m_ImageClassStatistic[i].size();
    meanStat.m_FalsePositive /= m_ImageClassStatistic[i].size();
    meanStat.m_FalseNegative /=  m_ImageClassStatistic[i].size();

    out << "|--------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|--------------|" << std::endl;
    out << std::setw(7) << "| Mean "<< "  | ";
    out << std::setw(11) << meanStat.m_DICE << " | ";
    out << std::setw(11) << meanStat.m_Jaccard << " | ";
    out << std::setw(11) << meanStat.m_Sensitivity  << " | ";
    out << std::setw(11) << meanStat.m_Specificity << " | ";
    out << std::setw(11) << meanStat.m_TruePositive << " | ";
    out << std::setw(11) << meanStat.m_TrueNegative<< " | ";
    out << std::setw(11) << meanStat.m_FalsePositive<< " | ";
    out << std::setw(11) << meanStat.m_FalseNegative << "  |"<< std::endl;
    sout << meanStat.m_DICE << ";";
    sout << meanStat.m_Jaccard << ";";
    sout << meanStat.m_Sensitivity  << ";";
    sout << meanStat.m_Specificity << ";";
    sout << meanStat.m_TruePositive << ";";
    sout << meanStat.m_TrueNegative<< ";";
    sout << meanStat.m_FalsePositive<< ";";
    sout << meanStat.m_FalseNegative << ";";

    wMeanStat.m_DICE /=  pointsSum;
    wMeanStat.m_Jaccard /=  pointsSum;
    wMeanStat.m_Sensitivity /=  pointsSum;
    wMeanStat.m_Specificity /=  pointsSum;
    wMeanStat.m_TruePositive /=  pointsSum;
    wMeanStat.m_TrueNegative /= pointsSum;
    wMeanStat.m_FalsePositive /= pointsSum;
    wMeanStat.m_FalseNegative /=  pointsSum;

    out << std::setw(7) << "| W-Mean"<< " | ";
    out << std::setw(11) << wMeanStat.m_DICE << " | ";
    out << std::setw(11) << wMeanStat.m_Jaccard << " | ";
    out << std::setw(11) << wMeanStat.m_Sensitivity  << " | ";
    out << std::setw(11) << wMeanStat.m_Specificity << " | ";
    out << std::setw(11) << wMeanStat.m_TruePositive << " | ";
    out << std::setw(11) << wMeanStat.m_TrueNegative<< " | ";
    out << std::setw(11) << wMeanStat.m_FalsePositive<< " | ";
    out << std::setw(11) << wMeanStat.m_FalseNegative << "  |"<< std::endl;
    sout << wMeanStat.m_DICE << ";";
    sout << wMeanStat.m_Jaccard << ";";
    sout << wMeanStat.m_Sensitivity  << ";";
    sout << wMeanStat.m_Specificity << ";";
    sout << wMeanStat.m_TruePositive << ";";
    sout << wMeanStat.m_TrueNegative<< ";";
    sout << wMeanStat.m_FalsePositive<< ";";
    sout << wMeanStat.m_FalseNegative << ";";

    m_ImageStatistic[i].m_Sensitivity = std::max(0.0,(1.0 * m_ImageStatistic[i].m_TruePositive) / (m_ImageStatistic[i].m_TruePositive + m_ImageStatistic[i].m_FalseNegative));
    m_ImageStatistic[i].m_Specificity = std::max(0.0,(1.0 * m_ImageStatistic[i].m_TrueNegative)  / ( m_ImageStatistic[i].m_FalsePositive + m_ImageStatistic[i].m_TrueNegative));

    out << std::setw(7) << "| Compl."<< " | ";
    out << std::setw(11) << "     x     " << " | ";
    out << std::setw(11) << "     x     " << " | ";
    out << std::setw(11) << m_ImageStatistic[i].m_Sensitivity  << " | ";
    out << std::setw(11) << "     x     " << " | ";
    out << std::setw(11) << m_ImageStatistic[i].m_TruePositive << " | ";
    out << std::setw(11) << m_ImageStatistic[i].m_TrueNegative<< " | ";
    out << std::setw(11) << m_ImageStatistic[i].m_FalsePositive<< " | ";
    out << std::setw(11) << m_ImageStatistic[i].m_FalseNegative << "  |"<< std::endl;
    out << "|--------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|--------------|" << std::endl;

    sout << m_ImageStatistic[i].m_Sensitivity  << ";";
    sout << m_ImageStatistic[i].m_TruePositive << ";";
    sout << m_ImageStatistic[i].m_FalsePositive<< std::endl;
    out << std::endl;
  }
}