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

#include "QmitkHistogramVisualizationWidget.h"

#include <QClipboard>

QmitkHistogramVisualizationWidget::QmitkHistogramVisualizationWidget(QWidget* parent) : QWidget(parent)
{
	m_Controls.setupUi(this);
  m_Controls.checkBoxShowSubchart->setChecked(false);
  m_Controls.spinBoxNBins->setValue(m_DefaultNBins);
  m_Controls.spinBoxNBins->setMinimum(m_MinNBins);
  m_Controls.spinBoxNBins->setMaximum(m_MaxNBins);
  m_Controls.spinBoxMaxValue->setMaximum(m_MaxNBins);
  m_Controls.spinBoxMaxValue->setMinimum(m_MinNBins);
  m_Controls.spinBoxMinValue->setMaximum(m_MaxNBins);
  m_Controls.spinBoxMinValue->setMinimum(m_MinNBins);

  CreateConnections();
}

void QmitkHistogramVisualizationWidget::SetHistogram(itk::Statistics::Histogram<double>::ConstPointer histogram, const std::string& dataLabel)
{
	if (histogram == nullptr)
		return;

	m_Histogram = histogram;
	m_Controls.chartWidget->AddData2D(ConvertHistogramToMap(m_Histogram), dataLabel);
	m_Controls.chartWidget->SetChartType(dataLabel, QmitkChartWidget::ChartType::bar);
	m_Controls.chartWidget->SetXAxisLabel("Gray value");
	m_Controls.chartWidget->SetYAxisLabel("Frequency");
  m_Controls.chartWidget->Show(m_Controls.checkBoxShowSubchart->isChecked());
	SetGUIElementsEnabled(true);
}

void QmitkHistogramVisualizationWidget::Reset()
{
  m_Controls.chartWidget->Clear();
	SetGUIElementsEnabled(false);
}

void QmitkHistogramVisualizationWidget::SetTheme(QmitkChartWidget::ColorTheme style)
{
  m_Controls.chartWidget->SetTheme(style);
}

void QmitkHistogramVisualizationWidget::CreateConnections()
{
	connect(m_Controls.buttonCopyHistogramToClipboard, &QPushButton::clicked, this, &QmitkHistogramVisualizationWidget::OnClipboardButtonClicked);
	connect(m_Controls.checkBoxUseDefaultNBins, &QCheckBox::clicked, this, &QmitkHistogramVisualizationWidget::OnDefaultNBinsCheckBoxChanged);
	connect(m_Controls.spinBoxNBins, &QSpinBox::editingFinished, this, &QmitkHistogramVisualizationWidget::OnNBinsSpinBoxValueChanged);
	connect(m_Controls.checkBoxShowSubchart, &QCheckBox::clicked, this, &QmitkHistogramVisualizationWidget::OnShowSubchartCheckBoxChanged);
	connect(m_Controls.checkBoxOverwriteMinMax, &QCheckBox::clicked, this, &QmitkHistogramVisualizationWidget::OnOverwriteMinMaxCheckBoxChanged);
	connect(m_Controls.spinBoxMaxValue, &QSpinBox::editingFinished, this, &QmitkHistogramVisualizationWidget::OnMaxValueSpinBoxValueChanged);
	connect(m_Controls.spinBoxMinValue, &QSpinBox::editingFinished, this, &QmitkHistogramVisualizationWidget::OnMinValueSpinBoxValueChanged);
}

void QmitkHistogramVisualizationWidget::SetGUIElementsEnabled(bool enabled)
{
	this->setEnabled(enabled);
	m_Controls.tabWidgetPlot->setEnabled(enabled);
	m_Controls.checkBoxShowSubchart->setEnabled(enabled);
	m_Controls.checkBoxUseDefaultNBins->setEnabled(enabled);
	m_Controls.spinBoxNBins->setEnabled(!m_Controls.checkBoxUseDefaultNBins->isChecked());
	m_Controls.buttonCopyHistogramToClipboard->setEnabled(enabled);
	m_Controls.checkBoxOverwriteMinMax->setEnabled(enabled);
	m_Controls.spinBoxMaxValue->setEnabled(m_Controls.checkBoxOverwriteMinMax->isChecked());
	m_Controls.spinBoxMinValue->setEnabled(m_Controls.checkBoxOverwriteMinMax->isChecked());
}

std::map<double, double> QmitkHistogramVisualizationWidget::ConvertHistogramToMap(itk::Statistics::Histogram<double>::ConstPointer histogram) const
{
	std::map<double, double> histogramMap;
	if (histogram)
	{
		auto endIt = histogram->End();
		auto it = histogram->Begin();

		// generating Lists of measurement and frequencies
		for (; it != endIt; ++it)
		{
			double frequency = it.GetFrequency();
			double measurement = it.GetMeasurementVector()[0];
			histogramMap.emplace(measurement, frequency);
		}

	}
	return histogramMap;
}

void QmitkHistogramVisualizationWidget::OnClipboardButtonClicked()
{
	if (m_Histogram)
	{
		QApplication::clipboard()->clear();
		QString clipboard("Measurement \t Frequency\n");
		auto iter = m_Histogram->Begin();
		auto iterEnd = m_Histogram->End();
		for (; iter != iterEnd; ++iter)
		{
			clipboard = clipboard.append("%L1 \t %L2\n")
				.arg(iter.GetMeasurementVector()[0], 0, 'f', 2)
				.arg(iter.GetFrequency());
		}

		QApplication::clipboard()->setText(clipboard, QClipboard::Clipboard);
	}
}

void QmitkHistogramVisualizationWidget::OnDefaultNBinsCheckBoxChanged()
{
	if (m_Controls.checkBoxUseDefaultNBins->isChecked())
	{
		m_Controls.spinBoxNBins->setEnabled(false);
    if (m_Controls.spinBoxNBins->value() != static_cast<int>(m_DefaultNBins) ) {
      m_Controls.spinBoxNBins->setValue(m_DefaultNBins);
      OnNBinsSpinBoxValueChanged();
    }
	}
	else
	{
		m_Controls.spinBoxNBins->setEnabled(true);
	}
}

void QmitkHistogramVisualizationWidget::OnNBinsSpinBoxValueChanged()
{
	emit RequestHistogramUpdate(m_Controls.spinBoxNBins->value());
}

void QmitkHistogramVisualizationWidget::OnShowSubchartCheckBoxChanged()
{
	m_Controls.chartWidget->Show(m_Controls.checkBoxShowSubchart->isChecked());
}

void QmitkHistogramVisualizationWidget::OnOverwriteMinMaxCheckBoxChanged()
{
	if (!m_Controls.checkBoxOverwriteMinMax->isChecked())
	{
		m_Controls.spinBoxMaxValue->setEnabled(false);
		m_Controls.spinBoxMinValue->setEnabled(false);
	}
	else
	{
		m_Controls.spinBoxMaxValue->setEnabled(true);
		m_Controls.spinBoxMinValue->setEnabled(true);
	}
}

void QmitkHistogramVisualizationWidget::OnMaxValueSpinBoxValueChanged()
{
	
}

void QmitkHistogramVisualizationWidget::OnMinValueSpinBoxValueChanged()
{
	
}