/*---------------------------------------------------------------------------------------------
* Copyright (c) Bentley Systems, Incorporated. All rights reserved.
* See LICENSE.md in the repository root for full copyright notice.
*--------------------------------------------------------------------------------------------*/
#include <ECPresentationPch.h>
#include "../Shared/Queries/CustomFunctions.h"
#include "../Shared/Queries/QueryBuilderHelpers.h"
#include "NavigationQuery.h"

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
template<typename T>
static T PrepareDisplayLabelField(T field)
    {
    field->SetGroupingClause(QueryBuilderHelpers::CreateDisplayLabelValueClause(field->GetName()));
    field->SetResultType(PresentationQueryFieldType::LabelDefinition);
    return field;
    }

Utf8CP NavigationQueryContract::ContractIdFieldName = "/ContractId/";
Utf8CP NavigationQueryContract::SpecificationIdentifierFieldName = "/SpecificationIdentifier/";
Utf8CP NavigationQueryContract::SkippedInstanceKeysFieldName = "/SkippedInstanceKeys/";
Utf8CP NavigationQueryContract::SkippedInstanceKeysInternalFieldName = NavigationQueryContract::SkippedInstanceKeysFieldName;
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> NavigationQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields;

    if (0 != GetId())
        fields.push_back(PresentationQueryContractSimpleField::Create(ContractIdFieldName, std::to_string(GetId()).c_str(), false));
    else
        fields.push_back(PresentationQueryContractSimpleField::Create(nullptr, ContractIdFieldName, false));

    fields.push_back(PresentationQueryContractSimpleField::Create(SpecificationIdentifierFieldName, Utf8PrintfString("'%s'", m_specificationIdentifier.c_str()), false));

#ifdef wip_skipped_instance_keys_performance_issue
    if (m_skippedInstanceKeysInternalField.IsNull())
        {
        bvector<Utf8String> args;
        if (m_pathFromSelectToParentClass.size() > 1)
            {
            for (size_t i = 0; i < m_pathFromSelectToParentClass.size() - 1; ++i)
                {
                RelatedClassCR related = m_pathFromSelectToParentClass[i];
                if (!related.GetRelationship().IsValid())
                    continue;

                Utf8String classIdClause, instanceIdClause;
                if (IsManyToManyRelationship(related.GetRelationship().GetClass()) || !HasNavigationProperty(related))
                    {
                    DIAGNOSTICS_ASSERT_SOFT(DiagnosticsCategory::Default, !related.GetRelationship().GetAlias().empty(), Utf8PrintfString("Expected relationship '%s' to have an alias", related.GetRelationship().GetClass().GetFullName()));
                    Utf8String sourceOrTarget = related.IsForwardRelationship() ? "Target" : "Source";
                    classIdClause = QueryHelpers::Wrap(related.GetRelationship().GetAlias()).append(".").append(QueryHelpers::Wrap(Utf8String(sourceOrTarget).append("ECClassId")));
                    instanceIdClause = QueryHelpers::Wrap(related.GetRelationship().GetAlias()).append(".").append(QueryHelpers::Wrap(Utf8String(sourceOrTarget).append("ECInstanceId")));
                    }
                else
                    {
                    DIAGNOSTICS_ASSERT_SOFT(DiagnosticsCategory::Default, !related.GetTargetClass().GetAlias().empty(), Utf8PrintfString("Expected target class '%s' to have an alias", related.GetTargetClass().GetClass().GetFullName()));
                    classIdClause = QueryHelpers::Wrap(related.GetTargetClass().GetAlias()).append(".").append(QueryHelpers::Wrap("ECClassId"));
                    instanceIdClause = QueryHelpers::Wrap(related.GetTargetClass().GetAlias()).append(".").append(QueryHelpers::Wrap("ECInstanceId"));
                    }
                args.push_back(classIdClause);
                args.push_back(instanceIdClause);
                }
            }
        m_skippedInstanceKeysInternalField = PresentationQueryContractFunctionField::Create(SkippedInstanceKeysInternalFieldName, FUNCTION_NAME_ECInstanceKeysArray,
            CreateFieldsList(args, false), false, false, FieldVisibility::Inner);
        }
    if (m_skippedInstanceKeysField.IsNull())
        {
        m_skippedInstanceKeysField = PresentationQueryContractFunctionField::Create(SkippedInstanceKeysFieldName, FUNCTION_NAME_AggregateJsonArray,
            { m_skippedInstanceKeysInternalField }, false, true, FieldVisibility::Both);
        }
    fields.push_back(m_skippedInstanceKeysField);
    fields.push_back(m_skippedInstanceKeysInternalField);
#endif

    return fields;
    }

Utf8CP ECInstanceNodesQueryContract::ECInstanceIdFieldName = "/ECInstanceId/";
Utf8CP ECInstanceNodesQueryContract::ECClassIdFieldName = "/ECClassId/";
Utf8CP ECInstanceNodesQueryContract::DisplayLabelFieldName = "/DisplayLabel/";
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
ECInstanceNodesQueryContract::ECInstanceNodesQueryContract(uint64_t id, Utf8String specificationIdentifier, PresentationQueryBuilderCR instanceKeysSelectQuery,
    ECClassCP ecClass, PresentationQueryContractFieldPtr displayLabelField, bvector<RelatedClassPath> const& relatedInstancePaths)
    : T_Super(id, specificationIdentifier, instanceKeysSelectQuery)
    {
    m_ecInstanceIdField = PresentationQueryContractSimpleField::Create(ECInstanceIdFieldName, "ECInstanceId");
    m_ecClassIdField = PresentationQueryContractSimpleField::Create(ECClassIdFieldName, "ECClassId");
    m_relatedInstanceInfoField = CreateRelatedInstanceInfoField(relatedInstancePaths);
    m_displayLabelField = displayLabelField;
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void ECInstanceNodesQueryContract::_SetECInstanceIdFieldName(Utf8CP name)
    {
    m_ecInstanceIdField->SetClause(name);
    m_ecInstanceIdField->SetName(name);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void ECInstanceNodesQueryContract::_SetECClassIdFieldName(Utf8CP name)
    {
    m_ecClassIdField->SetClause(name);
    m_ecClassIdField->SetName(name);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> ECInstanceNodesQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields = T_Super::_GetFields();
    fields.push_back(m_ecInstanceIdField);
    fields.push_back(m_ecClassIdField);
    fields.push_back(m_displayLabelField);
    fields.push_back(m_relatedInstanceInfoField);
    return fields;
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryBuilderPtr ECInstanceNodesQueryContract::CreateInstanceKeysSelectQuery(ECInstanceKey const& key) const
    {
    Utf8PrintfString query("SELECT ? AS [%s], ? AS [%s]", ECClassIdFieldName, ECInstanceIdFieldName);
    BoundQueryValuesList bindings(
        {
        std::make_shared<BoundQueryId>(key.GetClassId()),
        std::make_shared<BoundQueryId>(key.GetInstanceId())
        });
    return StringQueryBuilder::Create(query, bindings);
    }

Utf8CP MultiECInstanceNodesQueryContract::ECInstanceIdFieldName = "/ECInstanceId/";
Utf8CP MultiECInstanceNodesQueryContract::ECClassIdFieldName = "/ECClassId/";
Utf8CP MultiECInstanceNodesQueryContract::InstanceKeysFieldName = "/InstanceKey/";
Utf8CP MultiECInstanceNodesQueryContract::DisplayLabelFieldName = "/DisplayLabel/";
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
MultiECInstanceNodesQueryContract::MultiECInstanceNodesQueryContract(uint64_t id, Utf8String specificationIdentifier, PresentationQueryBuilderCR instanceKeysSelectQuery,
    ECClassCP ecClass, PresentationQueryContractFieldPtr displayLabelField, bool aggregateInstanceKeys, bvector<RelatedClassPath> const& relatedInstancePaths)
    : T_Super(id, specificationIdentifier, instanceKeysSelectQuery)
    {
    m_ecInstanceIdField = PresentationQueryContractSimpleField::Create(ECInstanceIdFieldName, "ECInstanceId", true, false, FieldVisibility::Inner);
    m_ecClassIdField = PresentationQueryContractSimpleField::Create(ECClassIdFieldName, "ECClassId", true, false, FieldVisibility::Inner);
    m_relatedInstanceInfoField = CreateRelatedInstanceInfoField(relatedInstancePaths);
    m_displayLabelField = displayLabelField;
    if (aggregateInstanceKeys)
        {
        m_instanceKeysField = PresentationQueryContractFunctionField::Create(InstanceKeysFieldName, FUNCTION_NAME_GetInstanceKeys, {m_ecClassIdField, m_ecInstanceIdField}, false, true);
        m_includeIdFields = true;
        }
    else
        {
        m_instanceKeysField = PresentationQueryContractFunctionField::Create(InstanceKeysFieldName, FUNCTION_NAME_GetInstanceKey, {m_ecClassIdField, m_ecInstanceIdField});
        m_includeIdFields = false;
        }
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void MultiECInstanceNodesQueryContract::_SetECInstanceIdFieldName(Utf8CP name)
    {
    m_ecInstanceIdField->SetClause(name);
    m_ecInstanceIdField->SetName(name);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void MultiECInstanceNodesQueryContract::_SetECClassIdFieldName(Utf8CP name)
    {
    m_ecClassIdField->SetClause(name);
    m_ecClassIdField->SetName(name);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> MultiECInstanceNodesQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields = T_Super::_GetFields();
    if (m_includeIdFields)
        {
        fields.push_back(m_ecInstanceIdField);
        fields.push_back(m_ecClassIdField);
        }
    fields.push_back(m_displayLabelField);
    fields.push_back(m_relatedInstanceInfoField);
    fields.push_back(m_instanceKeysField);
    return fields;
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryBuilderPtr MultiECInstanceNodesQueryContract::CreateInstanceKeysSelectQuery(bvector<ECInstanceKey> const& keys) const
    {
    static Utf8PrintfString const s_keySelectTemplate("SELECT ? AS [%s], ? AS [%s]", ECClassIdFieldName, ECInstanceIdFieldName);
    Utf8String query;
    BoundQueryValuesList bindings;
    for (auto const& key : keys)
        {
        if (!query.empty())
            query.append(" UNION ALL ");
        query.append(s_keySelectTemplate);
        bindings.push_back(std::make_shared<BoundQueryId>(key.GetClassId()));
        bindings.push_back(std::make_shared<BoundQueryId>(key.GetInstanceId()));
        }
    return StringQueryBuilder::Create(query, bindings);
    }

Utf8CP ECClassGroupingNodesQueryContract::ECInstanceIdFieldName = "/ECInstanceId/";
Utf8CP ECClassGroupingNodesQueryContract::ECClassIdFieldName = "/ECClassId/";
Utf8CP ECClassGroupingNodesQueryContract::IsClassPolymorphicFieldName = "/IsClassPolymorphic/";
Utf8CP ECClassGroupingNodesQueryContract::DisplayLabelFieldName = "/DisplayLabel/";
Utf8CP ECClassGroupingNodesQueryContract::GroupedInstancesCountFieldName = "/GroupedInstancesCount/";
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
ECClassGroupingNodesQueryContract::ECClassGroupingNodesQueryContract(uint64_t id, Utf8String specificationIdentifier, PresentationQueryBuilderCR instanceKeysSelectQuery, ECClassId customClassId, bool isPolymorphic)
    : T_Super(id, specificationIdentifier, instanceKeysSelectQuery), m_customClassId(customClassId), m_isPolymorphic(isPolymorphic)
    {
    auto actualClassIdField = PresentationQueryContractSimpleField::Create(ECClassIdFieldName, "ECClassId");
    m_ecClassIdField = customClassId.IsValid() ? PresentationQueryContractSimpleField::Create(ECClassIdFieldName, customClassId.ToString(), false) : actualClassIdField;
    m_groupedInstancesCountField = PresentationQueryContractSimpleField::Create(GroupedInstancesCountFieldName, "COUNT(1)", false, true);
    m_displayLabelField = PrepareDisplayLabelField(PresentationQueryContractFunctionField::Create(DisplayLabelFieldName, FUNCTION_NAME_GetECClassDisplayLabel,
        {m_ecClassIdField, m_groupedInstancesCountField},
        true, true, FieldVisibility::Outer));
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void ECClassGroupingNodesQueryContract::_SetECClassIdFieldName(Utf8CP name)
    {
    m_ecClassIdField->SetClause(name);
    m_ecClassIdField->SetName(name);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> ECClassGroupingNodesQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields = T_Super::_GetFields();
    fields.push_back(m_ecClassIdField);
    fields.push_back(PresentationQueryContractSimpleField::Create(IsClassPolymorphicFieldName, m_isPolymorphic ? "TRUE" : "FALSE", false));
    fields.push_back(m_displayLabelField);
    fields.push_back(m_groupedInstancesCountField);
    return fields;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryBuilderPtr ECClassGroupingNodesQueryContract::CreateInstanceKeysSelectQuery(ECClassCR ecClass, bool isPolymorphic) const
    {
    auto query = ComplexQueryBuilder::Create();
    query->SelectAll();
    query->From(*GetInstanceKeysSelectQuery().Clone());
    query->Where(Utf8PrintfString("[%s] IS (%s%s)", ECClassIdFieldName, isPolymorphic ? "" : "ONLY ", ecClass.GetFullName()).c_str(), BoundQueryValuesList());
    return query;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> ECClassGroupedInstancesQueryContract::_GetFields() const
    {
    return T_Super::_GetFields();
    }

Utf8CP DisplayLabelGroupingNodesQueryContract::ECInstanceIdFieldName = "/ECInstanceId/";
Utf8CP DisplayLabelGroupingNodesQueryContract::ECClassIdFieldName = "/ECClassId/";
Utf8CP DisplayLabelGroupingNodesQueryContract::DisplayLabelFieldName = "/DisplayLabel/";
Utf8CP DisplayLabelGroupingNodesQueryContract::GroupedInstancesCountFieldName = "/GroupedInstancesCount/";
Utf8CP DisplayLabelGroupingNodesQueryContract::GroupedInstanceKeysFieldName = "/GroupedInstanceKeys/";
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
DisplayLabelGroupingNodesQueryContract::DisplayLabelGroupingNodesQueryContract(uint64_t id, Utf8String specificationIdentifier, PresentationQueryBuilderCR instanceKeysSelectQuery, ECClassCP ecClass, PresentationQueryContractFieldPtr displayLabelField)
    : T_Super(id, specificationIdentifier, instanceKeysSelectQuery)
    {
    auto maxInstanceKeysField = PresentationQueryContractSimpleField::Create("", std::to_string(MAX_LABEL_GROUPED_INSTANCE_KEYS).c_str(), false, false, FieldVisibility::Inner);
    m_ecClassIdField = PresentationQueryContractSimpleField::Create(ECClassIdFieldName, "ECClassId", true, false, FieldVisibility::Inner);
    m_ecInstanceIdField = PresentationQueryContractSimpleField::Create(ECInstanceIdFieldName, "ECInstanceId", true, false, FieldVisibility::Inner);
    m_groupedInstanceKeysField = PresentationQueryContractFunctionField::Create(GroupedInstanceKeysFieldName,
        FUNCTION_NAME_GetLimitedInstanceKeys, { maxInstanceKeysField, m_ecClassIdField, m_ecInstanceIdField }, false, true);
    m_groupedInstancesCountField = PresentationQueryContractSimpleField::Create(GroupedInstancesCountFieldName, "COUNT(1)", false, true);
    m_displayLabelField = displayLabelField;
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void DisplayLabelGroupingNodesQueryContract::_SetECInstanceIdFieldName(Utf8CP name)
    {
    m_ecInstanceIdField->SetClause(name);
    m_ecInstanceIdField->SetName(name);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void DisplayLabelGroupingNodesQueryContract::_SetECClassIdFieldName(Utf8CP name)
    {
    m_ecClassIdField->SetClause(name);
    m_ecClassIdField->SetName(name);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> DisplayLabelGroupingNodesQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields = T_Super::_GetFields();
    fields.push_back(m_ecClassIdField);
    fields.push_back(m_ecInstanceIdField);
    fields.push_back(m_displayLabelField);
    fields.push_back(m_groupedInstancesCountField);
    fields.push_back(m_groupedInstanceKeysField);
    return fields;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryBuilderPtr DisplayLabelGroupingNodesQueryContract::CreateInstanceKeysSelectQuery(LabelDefinitionCR label) const
    {
    auto query = ComplexQueryBuilder::Create();
    query->SelectAll();
    query->From(*GetInstanceKeysSelectQuery().Clone());
    query->Where(Utf8PrintfString("json_extract([%s], '$.DisplayValue') = ?", DisplayLabelGroupingNodesQueryContract::DisplayLabelFieldName).c_str(),
        BoundQueryValuesList({ std::make_shared<BoundQueryECValue>(ECValue(label.GetDisplayValue().c_str())) }));
    return query;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> DisplayLabelGroupedInstancesQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields = T_Super::_GetFields();
    fields.push_back(m_displayLabelField);
    return fields;
    }

Utf8CP ECPropertyGroupingNodesQueryContract::ECInstanceIdFieldName = "/ECInstanceId/";
Utf8CP ECPropertyGroupingNodesQueryContract::ECClassIdFieldName = "/ECClassId/";
Utf8CP ECPropertyGroupingNodesQueryContract::ECPropertyClassIdFieldName = "/ECPropertyClassId/";
Utf8CP ECPropertyGroupingNodesQueryContract::ECPropertyNameFieldName = "/PropertyName/";
Utf8CP ECPropertyGroupingNodesQueryContract::DisplayLabelFieldName = "/DisplayLabel/";
Utf8CP ECPropertyGroupingNodesQueryContract::ImageIdFieldName = "/ImageId/";
Utf8CP ECPropertyGroupingNodesQueryContract::GroupingValueFieldName = "/GroupingValue/";
Utf8CP ECPropertyGroupingNodesQueryContract::GroupingValuesFieldName = "/GroupingValues/";
Utf8CP ECPropertyGroupingNodesQueryContract::IsRangeFieldName = "/IsRange/";
Utf8CP ECPropertyGroupingNodesQueryContract::GroupedInstancesCountFieldName = "/GroupedInstancesCount/";
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
ECPropertyGroupingNodesQueryContract::ECPropertyGroupingNodesQueryContract(uint64_t id, Utf8String specificationIdentifier, PresentationQueryBuilderCR instanceKeysSelectQuery, SelectClass<ECClass> const& propertyClass, ECPropertyCR prop, PropertyGroupCR spec, SelectClass<ECClass> const* foreignKeyClass)
    : T_Super(id, specificationIdentifier, instanceKeysSelectQuery), m_property(prop), m_specification(spec), m_groupingPropertyClassAlias(propertyClass.GetAlias())
    {
    m_ecInstanceIdField = PresentationQueryContractSimpleField::Create(ECInstanceIdFieldName, "ECInstanceId", true, false, FieldVisibility::Inner);
    m_ecClassIdField = PresentationQueryContractSimpleField::Create(ECClassIdFieldName, "ECClassId", true, false, FieldVisibility::Inner);
    m_ecPropertyClassIdField = PresentationQueryContractSimpleField::Create(ECPropertyClassIdFieldName, propertyClass.GetClass().GetId().ToString().c_str(), false);
    m_ecPropertyNameField = PresentationQueryContractSimpleField::Create(ECPropertyNameFieldName, Utf8PrintfString("'%s'", m_property.GetName().c_str()).c_str(), false);
    auto propertyDisplayValueField = CreatePropertyDisplayValueField(prop, propertyClass.GetAlias(), foreignKeyClass ? foreignKeyClass->GetAlias() : "", *m_ecClassIdField, *m_ecInstanceIdField);
    m_imageIdField = CreateImageIdField(m_specification, *propertyDisplayValueField);
    m_isRangeField = PresentationQueryContractSimpleField::Create(IsRangeFieldName, spec.GetRanges().empty() ? "false" : "true", false);
    m_groupingValueField = CreateGroupingValueField(prop, propertyClass.GetAlias(), spec, *propertyDisplayValueField);
    m_displayLabelField = CreateDisplayLabelField(m_specification, *m_groupingValueField, *propertyDisplayValueField, *m_ecPropertyClassIdField, *m_ecPropertyNameField, *m_ecInstanceIdField);
    }
/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
ECPropertyGroupingNodesQueryContract::ECPropertyGroupingNodesQueryContract(uint64_t id, Utf8String specificationIdentifier, PresentationQueryBuilderCR instanceKeysSelectQuery, ECPropertyCR prop, PropertyGroupCR spec)
    : T_Super(id, specificationIdentifier, instanceKeysSelectQuery), m_property(prop), m_specification(spec)
    {
    m_ecInstanceIdField = PresentationQueryContractSimpleField::Create(ECInstanceIdFieldName, "", true, false, FieldVisibility::Inner);
    m_ecClassIdField = PresentationQueryContractSimpleField::Create(ECClassIdFieldName, "", true, false, FieldVisibility::Inner);
    m_ecPropertyClassIdField = PresentationQueryContractSimpleField::Create(ECPropertyClassIdFieldName, "", false);
    m_ecPropertyNameField = PresentationQueryContractSimpleField::Create(ECPropertyNameFieldName, "", false);
    m_imageIdField = PresentationQueryContractSimpleField::Create(ImageIdFieldName, "", false);
    m_isRangeField = PresentationQueryContractSimpleField::Create(IsRangeFieldName, "", false);
    m_groupedInstancesCountField = PresentationQueryContractSimpleField::Create(GroupedInstancesCountFieldName, "COUNT(1)", false, true);
    m_groupingValueField = PresentationQueryContractSimpleField::Create(GroupingValueFieldName, "", false, false, (PropertyGroupingValue::PropertyValue == spec.GetPropertyGroupingValue()) ? FieldVisibility::Both : FieldVisibility::Inner);
    m_groupingValuesField = CreateGroupingValuesField(prop, spec, *m_groupingValueField);
    m_displayLabelField = CreateGroupingDisplayLabelField(m_specification, *m_groupingValueField, *m_ecPropertyClassIdField, *m_ecPropertyNameField, *m_ecInstanceIdField);
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryContractFieldPtr ECPropertyGroupingNodesQueryContract::CreatePropertyDisplayValueField(ECPropertyCR prop, Utf8StringCR propertyClassAlias, Utf8StringCR foreignKeyClassAlias,
    PresentationQueryContractFieldCR ecClassIdField, PresentationQueryContractFieldCR ecInstanceIdField)
    {
    if (prop.GetIsNavigation())
        {
        auto field = PresentationQueryContractFunctionField::Create("", FUNCTION_NAME_GetNavigationPropertyLabel,
            {
            &ecClassIdField,
            &ecInstanceIdField,
            });
        field->SetPrefixOverride(foreignKeyClassAlias);
        return field;
        }

    if (prop.GetIsPrimitive() && PRIMITIVETYPE_Point3d == prop.GetAsPrimitiveProperty()->GetType())
        return QueryContractHelpers::CreatePointAsJsonStringSelectField(prop.GetName(), propertyClassAlias, 3);

    if (prop.GetIsPrimitive() && PRIMITIVETYPE_Point2d == prop.GetAsPrimitiveProperty()->GetType())
        return QueryContractHelpers::CreatePointAsJsonStringSelectField(prop.GetName(), propertyClassAlias, 2);

    auto field = PresentationQueryContractSimpleField::Create("", prop.GetName().c_str());
    field->SetPrefixOverride(propertyClassAlias);
    return field;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryContractFieldPtr ECPropertyGroupingNodesQueryContract::CreateGroupingValueField(ECPropertyCR prop, Utf8StringCR propertyClassAlias, PropertyGroupCR spec, PresentationQueryContractFieldCR propertyDisplayValueField)
    {
    bool groupByValue = (PropertyGroupingValue::PropertyValue == spec.GetPropertyGroupingValue());
    if (prop.GetIsPrimitive() && (PRIMITIVETYPE_Point3d == prop.GetAsPrimitiveProperty()->GetType() || PRIMITIVETYPE_Point2d == prop.GetAsPrimitiveProperty()->GetType()))
        {
        return PresentationQueryContractSimpleField::Create(GroupingValueFieldName, propertyDisplayValueField.GetSelectClause(),
            false, false, groupByValue ? FieldVisibility::Both : FieldVisibility::Inner);
        }

    Utf8String groupingValueClause = QueryHelpers::Wrap(prop.GetName());
    if (prop.GetIsNavigation())
        groupingValueClause.append(".[Id]");

    auto field = PresentationQueryContractSimpleField::Create(GroupingValueFieldName, groupingValueClause.c_str(),
        true, false, groupByValue ? FieldVisibility::Both : FieldVisibility::Inner);
    field->SetPrefixOverride(propertyClassAlias);
    return field;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryContractFieldPtr ECPropertyGroupingNodesQueryContract::CreateGroupingValuesField(ECPropertyCR prop, PropertyGroupCR spec, PresentationQueryContractFieldCR groupingValueField)
    {
    PrimitiveType propertyType = prop.GetIsPrimitive() ? prop.GetAsPrimitiveProperty()->GetType()
        : prop.GetIsNavigation() ? PRIMITIVETYPE_Long : PRIMITIVETYPE_String;
    Utf8CP funcName = (spec.GetRanges().empty() && (propertyType == PRIMITIVETYPE_Point2d || propertyType == PRIMITIVETYPE_Point3d))
        ? FUNCTION_NAME_StringifiedJsonConcat : FUNCTION_NAME_JsonConcat;
    auto groupingValuesField = PresentationQueryContractFunctionField::Create(GroupingValuesFieldName, funcName, {}, true, true);
    groupingValuesField->SetDistinctArguments(true);
    if (spec.GetRanges().empty())
        {
        groupingValuesField->GetFunctionParameters().push_back(PresentationQueryContractFunctionField::Create("", FUNCTION_NAME_GetPropertyValueJson,
            {
            &groupingValueField,
            PresentationQueryContractSimpleField::Create("", std::to_string((int)propertyType).c_str(), false),
            }));
        }
    else
        {
        groupingValuesField->GetFunctionParameters().push_back(PresentationQueryContractFunctionField::Create("", FUNCTION_NAME_GetRangeIndex,
            {
            &groupingValueField,
            }));
        }
    return groupingValuesField;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryContractFieldPtr ECPropertyGroupingNodesQueryContract::CreateDisplayLabelField(PropertyGroupCR spec, PresentationQueryContractFieldCR groupingValueField,
    PresentationQueryContractFieldCR propertyDisplayValueField, PresentationQueryContractFieldCR ecPropertyClassIdField, PresentationQueryContractFieldCR ecPropertyNameField,
    PresentationQueryContractFieldCR ecInstanceIdField)
    {
    bool groupByValue = (PropertyGroupingValue::PropertyValue == spec.GetPropertyGroupingValue());

    PresentationQueryContractFieldCPtr defaultLabelField, groupedInstancesCountField;
    if (groupByValue)
        {
        defaultLabelField = &groupingValueField;
        groupedInstancesCountField = PresentationQueryContractSimpleField::Create(nullptr, "COUNT(1)", false);
        }
    else
        {
        defaultLabelField = PresentationQueryContractSimpleField::Create(GroupingValueFieldName, propertyDisplayValueField.GetSelectClause(),
            false, false, FieldVisibility::Inner);
        groupedInstancesCountField = PresentationQueryContractSimpleField::Create(nullptr, "0", false);
        }

    return PrepareDisplayLabelField(PresentationQueryContractFunctionField::Create(DisplayLabelFieldName, FUNCTION_NAME_GetECPropertyDisplayLabel,
        {&ecPropertyClassIdField, &ecPropertyNameField, &ecInstanceIdField, defaultLabelField, groupedInstancesCountField},
        true, groupByValue, groupByValue ? FieldVisibility::Outer : FieldVisibility::Both));
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryContractFieldPtr ECPropertyGroupingNodesQueryContract::CreateGroupingDisplayLabelField(PropertyGroupCR spec, PresentationQueryContractFieldCR groupingValueField,
    PresentationQueryContractFieldCR ecPropertyClassIdField, PresentationQueryContractFieldCR ecPropertyNameField, PresentationQueryContractFieldCR ecInstanceIdField)
    {
    bool groupByValue = (PropertyGroupingValue::PropertyValue == spec.GetPropertyGroupingValue());
    if (groupByValue)
        {
        return PrepareDisplayLabelField(PresentationQueryContractFunctionField::Create(DisplayLabelFieldName, FUNCTION_NAME_GetECPropertyDisplayLabel,
            { &ecPropertyClassIdField, &ecPropertyNameField, &ecInstanceIdField, &groupingValueField, PresentationQueryContractSimpleField::Create(nullptr, "COUNT(1)", false) },
            true, true));
        }
    return PrepareDisplayLabelField(PresentationQueryContractSimpleField::Create(DisplayLabelFieldName, "", false, false));
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryContractFieldPtr ECPropertyGroupingNodesQueryContract::CreateImageIdField(PropertyGroupCR spec, PresentationQueryContractFieldCR propertyDisplayValueField)
    {
    if (spec.GetRanges().empty())
        {
        if (spec.GetImageId().empty())
            return PresentationQueryContractSimpleField::Create(ImageIdFieldName, "''", false);
        return PresentationQueryContractSimpleField::Create(ImageIdFieldName, Utf8PrintfString("'%s'", spec.GetImageId().c_str()).c_str());
        }
    return PresentationQueryContractFunctionField::Create(ImageIdFieldName, FUNCTION_NAME_GetRangeImageId, { &propertyDisplayValueField });
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
void ECPropertyGroupingNodesQueryContract::_SetECInstanceIdFieldName(Utf8CP name)
    {
    m_ecInstanceIdField->SetClause(name);
    m_ecInstanceIdField->SetName(name);
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> ECPropertyGroupingNodesQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields = T_Super::_GetFields();
    fields.push_back(m_ecInstanceIdField);
    fields.push_back(m_ecPropertyNameField);
    fields.push_back(m_ecClassIdField);
    fields.push_back(m_ecPropertyClassIdField);
    fields.push_back(m_displayLabelField);
    fields.push_back(m_imageIdField);
    fields.push_back(m_isRangeField);
    fields.push_back(m_groupingValueField);
    if (m_groupingValuesField.IsValid())
        fields.push_back(m_groupingValuesField);
    if (m_groupedInstancesCountField.IsValid())
        fields.push_back(m_groupedInstancesCountField);
    return fields;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
PresentationQueryBuilderPtr ECPropertyGroupingNodesQueryContract::CreateInstanceKeysSelectQuery(RapidJsonValueCR groupingValuesJson) const
    {
    auto query = ComplexQueryBuilder::Create();
    query->SelectAll();
    query->From(*GetInstanceKeysSelectQuery().Clone());
    query->Where(QueryBuilderHelpers::CreatePropertyGroupFilteringClause(m_property, "[PropertyValue]",
        m_specification, groupingValuesJson));
    return query;
    }

/*---------------------------------------------------------------------------------**//**
* @bsimethod
+---------------+---------------+---------------+---------------+---------------+------*/
bvector<PresentationQueryContractFieldCPtr> ECPropertyGroupedInstancesQueryContract::_GetFields() const
    {
    bvector<PresentationQueryContractFieldCPtr> fields = T_Super::_GetFields();
    fields.push_back(PresentationQueryContractSimpleField::Create("PropertyValue", m_propertyValueSelector, false));
    return fields;
    }
