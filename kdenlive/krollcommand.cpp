/***************************************************************************
                          krollcommand  -  description
                             -------------------
    begin                : Wed Aug 25 2004
    copyright            : (C) 2004 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "krollcommand.h"
#include "docclipref.h"
#include "kdenlivedoc.h"

#include <klocale.h>
#include <kdebug.h>

namespace Command {

KRollCommand::KRollCommand(KdenliveDoc *doc, DocClipRef &clip)
{
	m_doc = doc;
	m_trackNum = clip.trackNum();
	m_end_trackEnd = m_start_trackEnd = clip.trackEnd();
	m_end_trackStart = m_start_trackStart = clip.trackStart();
	m_end_cropStart = m_start_cropStart = clip.cropStartTime();
}

KRollCommand::~KRollCommand()
{
}

void KRollCommand::setEndSize(DocClipRef &clip)
{
	m_end_trackEnd = clip.trackEnd();
	m_end_trackStart = clip.trackStart();
	m_end_cropStart = clip.cropStartTime();
}

/** Returns the name of this command */
QString KRollCommand::name() const
{
	return i18n("Roll clips");
}

/** Executes this command */
void KRollCommand::execute()
{
	DocClipRef *clip = m_doc->track(m_trackNum)->getClipAt( (m_start_trackStart + m_start_trackEnd) / 2.0);
	if(!clip) {
		kdWarning() << "RollCommand execute failed - cannot find clips!!!" << endl;
	} else {
		clip->setTrackStart(m_end_trackStart);
		clip->setCropStartTime(m_end_cropStart);
		clip->setTrackEnd(m_end_trackEnd);
	}
	m_doc->indirectlyModified();
}

/** Unexecutes this command */
void KRollCommand::unexecute()
{
	DocClipRef *clip = m_doc->track(m_trackNum)->getClipAt( m_end_trackStart + ((m_end_trackEnd - m_end_trackStart) / 2.0));
	if(!clip) {
		kdWarning() << "RollCommand unexecute failed - cannot find clips!!!" << endl;
	} else {
		clip->setTrackStart(m_start_trackStart);
		clip->setCropStartTime(m_start_cropStart);
		clip->setTrackEnd(m_start_trackEnd);
	}
	m_doc->indirectlyModified();
}

/** Sets the trackEnd() for the end destination to the time specified. */
void KRollCommand::setEndTrackEnd(const GenTime &time)
{
	m_end_trackEnd = time;
}

void KRollCommand::setEndTrackStart(const GenTime &time)
{
	m_end_trackStart = time;
}

} // namespace Command
