#include "EventMapper.h"
#include <string>
//XML Event
//##ModelId=3E788FC000E5
const std::string mitk::EventMapper::STYLE = "STYLE";
//##ModelId=3E788FC0025C
const std::string mitk::EventMapper::NAME = "NAME";
//##ModelId=3E788FC002AA
const std::string mitk::EventMapper::ID = "ID";
//##ModelId=3E77572901E9
const std::string mitk::EventMapper::TYPE = "TYPE";
//##ModelId=3E7757290256
const std::string mitk::EventMapper::BUTTON = "BUTTON";
//##ModelId=3E8B08FA01AA
const std::string mitk::EventMapper::BUTTONSTATE = "BUTTONSTATE";
//##ModelId=3E77572902C4
const std::string mitk::EventMapper::KEY = "KEY";

//for Linker ->no undefined reference
mitk::StateMachine *mitk::EventMapper::m_StateMachine;
EventDescriptionVec mitk::EventMapper::m_EventDescriptions;
mitk::StateEvent mitk::EventMapper::m_StateEvent;
std::string mitk::EventMapper::m_StyleName;

//##ModelId=3E5B349600CB
//##Documentation
//##
void mitk::EventMapper::SetStateMachine(StateMachine *stateMachine)
{
//was mit alter Statemachine-Zeiger?
	mitk::EventMapper::m_StateMachine = stateMachine;
}

//##ModelId=3E5B34CF0041
//##Documentation
//## searches the Event in m_EventDescription
//## and if included 
bool mitk::EventMapper::MapEvent(Event* event)
{
	if (m_StateMachine == NULL)
		return false;

	int i;
	for (i = 0;
		(i < ((int)(m_EventDescriptions.size()))) ||
		!(m_EventDescriptions[i] == *event);//insecure[] but .at(i) is not supported before std.vers 3.0
		i++){}
	if (!(m_EventDescriptions[i] == *event))//insecure
		//searched entry not found
		return false;

	m_StateEvent.Set( (m_EventDescriptions[i]).GetId(), event );
	m_StateMachine->HandleEvent(&m_StateEvent);
	return true;
}

//##ModelId=3E5B35140072
//##Documentation
//## loads an XML-File containing Events into m_EventDescriptions
//## also involved: EventMapper::startEvent(...)
bool mitk::EventMapper::LoadBehavior(std::string fileName)
{
	  if ( fileName.empty() )
       return false;

   QFile xmlFile( fileName.c_str() );
   if ( !xmlFile.exists() )
       return false;

   QXmlInputSource source( &xmlFile );
   QXmlSimpleReader reader;
   mitk::EventMapper* eventMapper = new EventMapper();
   reader.setContentHandler( eventMapper );
   reader.parse( source );

   delete eventMapper;
   return true;
}

//##ModelId=3E788FC00308
//##Documentation
//## reads a Tag from an XML-file
//## adds Events to m_EventDescription
bool mitk::EventMapper::startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& atts )
{

	qName.ascii(); //for debuging
	if( qName == "events")
	{
		m_StyleName = atts.value( STYLE.c_str() ).latin1();
	}
	else if ( qName == "event")
	{
		//neuen Eintrag in der m_EventDescriptions
		EventDescription tempEventDescr( atts.value( NAME.c_str() ).latin1(),
										 atts.value( ID.c_str() ).toInt(),
										 atts.value( TYPE.c_str() ).toInt(),
										 atts.value( BUTTON.c_str() ).toInt(),
										 atts.value( BUTTONSTATE.c_str() ).toInt(),
										 atts.value( KEY.c_str() ).toInt() );
		m_EventDescriptions.push_back(tempEventDescr);
	}
	return true;
}

//##ModelId=3E7B20EE01F5
std::string mitk::EventMapper::GetStyleName()
{
	return m_StyleName;
}