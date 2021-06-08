/*
 * Copyright (c) 2021, evilbat831
 * author: evilbat831
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "IndividualSampleModel.h"
#include "Profile.h"
#include <AK/StringBuilder.h>
#include <stdio.h>

namespace Profiler {

IndividualSampleModel::IndividualSampleModel(Profile& profile, size_t event_index)
    : m_profile(profile)
    , m_event_index(event_index)
{
}

IndividualSampleModel::~IndividualSampleModel()
{
}

int IndividualSampleModel::row_count(const GUI::ModelIndex&) const
{
    auto& event = m_profile.events().at(m_event_index);
    return event.frames.size();
}

int IndividualSampleModel::column_count(const GUI::ModelIndex&) const
{
    return Column::__Count;
}

String IndividualSampleModel::column_name(int column) const
{
    switch (column) {
    case Column::Address:
        return "Address";
    case Column::ObjectName:
        return "Object";
    case Column::Symbol:
        return "Symbol";
    default:
        VERIFY_NOT_REACHED();
    }
}

}
