/*---------------------------------------------------------------------------------------------
* Copyright (c) Bentley Systems, Incorporated. All rights reserved.
* See COPYRIGHT.md in the repository root for full copyright notice.
*--------------------------------------------------------------------------------------------*/
#include "ECDbPublishedTests.h"

USING_NAMESPACE_BENTLEY_EC
#include <ECDb/ConcurrentQueryManager.h>

BEGIN_ECDBUNITTESTS_NAMESPACE

struct InstanceReaderFixture : ECDbTestFixture {};

//---------------------------------------------------------------------------------------
// @bsimethod
//+---------------+---------------+---------------+---------------+---------------+------
TEST_F(InstanceReaderFixture, ecsql_read_instance) {
    ASSERT_EQ(BE_SQLITE_OK, SetupECDb("instanceReader.ecdb"));

    ECSqlStatement stmt;
    ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, R"sql(
        SELECT $ FROM meta.ECClassDef WHERE Description='Relates the property to its PropertyCategory.'
    )sql"));

    ASSERT_EQ(BE_SQLITE_ROW, stmt.Step());
    ASSERT_STREQ(stmt.GetNativeSql(), "SELECT extract_inst([ECClassDef].[ECClassId],[ECClassDef].[ECInstanceId]) FROM (SELECT [Id] ECInstanceId,32 ECClassId,[Description] FROM [main].[ec_Class]) [ECClassDef] WHERE [ECClassDef].[Description]='Relates the property to its PropertyCategory.'");
    ASSERT_STREQ(stmt.GetValueText(0), R"json({"ECInstanceId":"0x2e","ECClassId":"ECDbMeta.ECClassDef","Schema":{"Id":"0x4","RelECClassId":"ECDbMeta.SchemaOwnsClasses"},"Name":"PropertyHasCategory","Description":"Relates the property to its PropertyCategory.","Type":1,"Modifier":2,"RelationshipStrength":0,"RelationshipStrengthDirection":1})json");
}

//---------------------------------------------------------------------------------------
// @bsimethod
//+---------------+---------------+---------------+---------------+---------------+------
TEST_F(InstanceReaderFixture, ecsql_read_property) {
    ASSERT_EQ(BE_SQLITE_OK, SetupECDb("instanceReader.ecdb"));

    ECSqlStatement stmt;
    ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, R"sql(
        SELECT $ -> name FROM meta.ECClassDef WHERE Description='Relates the property to its PropertyCategory.'
    )sql"));

    ASSERT_EQ(BE_SQLITE_ROW, stmt.Step());
    ASSERT_STREQ(stmt.GetNativeSql(), "SELECT extract_prop([ECClassDef].[ECClassId],[ECClassDef].[ECInstanceId],'name') FROM (SELECT [Id] ECInstanceId,32 ECClassId,[Description] FROM [main].[ec_Class]) [ECClassDef] WHERE [ECClassDef].[Description]='Relates the property to its PropertyCategory.'");
    ASSERT_STREQ(stmt.GetValueText(0), "PropertyHasCategory");
}

//---------------------------------------------------------------------------------------
// @bsimethod
//+---------------+---------------+---------------+---------------+---------------+------
TEST_F(InstanceReaderFixture, instance_reader) {
    ASSERT_EQ(BE_SQLITE_OK, SetupECDb("instanceReader.ecdb"));

    ECSqlStatement stmt;
    ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, R"sql(
        SELECT ECClassId, ECInstanceId, EXTRACT_INST('meta.ecClassDef',ECInstanceId) FROM meta.ECClassDef WHERE Description='Relates the property to its PropertyCategory.'
    )sql"));

    BeJsDocument doc;
    doc.Parse(R"json({
        "ECInstanceId": "0x2e",
        "ECClassId": "ECDbMeta.ECClassDef",
        "Schema": {
            "Id": "0x4",
            "RelECClassId": "ECDbMeta.SchemaOwnsClasses"
        },
        "Name": "PropertyHasCategory",
        "Description": "Relates the property to its PropertyCategory.",
        "Type": 1,
        "Modifier": 2,
        "RelationshipStrength": 0,
        "RelationshipStrengthDirection": 1
    })json");
    auto& reader = m_ecdb.GetInstanceReader();
    if(stmt.Step() == BE_SQLITE_ROW) {
        ECInstanceKey instanceKey (stmt.GetValueId<ECClassId>(0), stmt.GetValueId<ECInstanceId>(1));
        auto pos = InstanceReader::Position(stmt.GetValueId<ECInstanceId>(1), stmt.GetValueId<ECClassId>(0));
        ASSERT_EQ(true, reader.Seek(pos,[&](InstanceReader::IRowContext const& row){
            EXPECT_STRCASEEQ(doc.Stringify(StringifyFormat::Indented).c_str(), row.GetJson().Stringify(StringifyFormat::Indented).c_str());
        }));
    }
    if ("use syntax to get full instance") {
        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, "SELECT ECClassId, ECInstanceId, $ FROM meta.ECClassDef WHERE Description='Relates the property to its PropertyCategory.'"));
        const auto expectedSQL = "SELECT [ECClassDef].[ECClassId],[ECClassDef].[ECInstanceId],extract_inst([ECClassDef].[ECClassId],[ECClassDef].[ECInstanceId]) FROM (SELECT [Id] ECInstanceId,32 ECClassId,[Description] FROM [main].[ec_Class]) [ECClassDef] WHERE [ECClassDef].[Description]='Relates the property to its PropertyCategory.'";
        EXPECT_STRCASEEQ(expectedSQL, stmt.GetNativeSql());
        if(stmt.Step() == BE_SQLITE_ROW) {
            BeJsDocument inst;
            inst.Parse(stmt.GetValueText(2));
            EXPECT_STRCASEEQ(doc.Stringify(StringifyFormat::Indented).c_str(), inst.Stringify(StringifyFormat::Indented).c_str());

        }
    }
    if ("use syntax to get full instance using alias") {
        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, "SELECT m.ECClassId, m.ECInstanceId, m.$ FROM meta.ECClassDef m WHERE m.Description='Relates the property to its PropertyCategory.'"));
        const auto expectedSQL = "SELECT [m].[ECClassId],[m].[ECInstanceId],extract_inst([m].[ECClassId],[m].[ECInstanceId]) FROM (SELECT [Id] ECInstanceId,32 ECClassId,[Description] FROM [main].[ec_Class]) [m] WHERE [m].[Description]='Relates the property to its PropertyCategory.'";
        EXPECT_STRCASEEQ(expectedSQL, stmt.GetNativeSql());
        if(stmt.Step() == BE_SQLITE_ROW) {
            BeJsDocument inst;
            inst.Parse(stmt.GetValueText(2));
            EXPECT_STRCASEEQ(doc.Stringify(StringifyFormat::Indented).c_str(), inst.Stringify(StringifyFormat::Indented).c_str());

        }
    }
}

//---------------------------------------------------------------------------------------
// @bsimethod
//+---------------+---------------+---------------+---------------+---------------+------
TEST_F(InstanceReaderFixture, extract_prop) {
    ASSERT_EQ(SUCCESS, SetupECDb("PROP_EXISTS.ecdb", SchemaItem(
        R"xml(<ECSchema schemaName='TestSchema' alias='ts' version='1.0' xmlns='http://www.bentley.com/schemas/Bentley.ECXML.3.1'>
                   <ECSchemaReference name="ECDbMap" version="02.00" alias="ecdbmap" />
                   <ECSchemaReference name="CoreCustomAttributes" version="01.00.00" alias="CoreCA" />
                    <ECEntityClass typeName="P">
                        <ECProperty propertyName="s"    typeName="string"  />
                        <ECProperty propertyName="i"    typeName="int"     />
                        <ECProperty propertyName="d"    typeName="double"  />
                        <ECProperty propertyName="p2d"  typeName="point2d" />
                        <ECProperty propertyName="p3d"  typeName="point3d" />
                        <ECProperty propertyName="bi"   typeName="binary"  />
                        <ECProperty propertyName="l"    typeName="long"    />
                        <ECProperty propertyName="dt"   typeName="dateTime">
                            <ECCustomAttributes>
                                <DateTimeInfo xmlns="CoreCustomAttributes.01.00.00">
                                    <DateTimeKind>Utc</DateTimeKind>
                                </DateTimeInfo>
                            </ECCustomAttributes>
                        </ECProperty>
                        <ECProperty propertyName="b"    typeName="boolean" />
                    </ECEntityClass>
               </ECSchema>)xml")));
    m_ecdb.SaveChanges();

    // sample primitive type
    const bool kB = true;
    const DateTime kDt = DateTime::GetCurrentTimeUtc();
    const double kD = 3.13;
    const DPoint2d kP2d = DPoint2d::From(2, 4);
    const DPoint3d kP3d = DPoint3d::From(4, 5, 6);
    const int kBiLen = 10;
    const int kI = 0x3432;
    const int64_t kL = 0xfefefefefefefefe;
    const uint8_t kBi[kBiLen] = {0x71, 0xdd, 0x83, 0x7d, 0x0b, 0xf2, 0x50, 0x01, 0x0a, 0xe1};
    const char* kText = "Hello, World";

    if ("insert a row with primitive value") {
        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success,
                stmt.Prepare(m_ecdb, "INSERT INTO ts.P(s,i,d,p2d,p3d,bi,l,dt,b) VALUES(?,?,?,?,?,?,?,?,?)"));

        // bind primitive types
        stmt.BindText(1, kText, IECSqlBinder::MakeCopy::No);
        stmt.BindInt(2, kI);
        stmt.BindDouble(3, kD);
        stmt.BindPoint2d(4, kP2d);
        stmt.BindPoint3d(5, kP3d);
        stmt.BindBlob(6, &kBi[0], kBiLen, IECSqlBinder::MakeCopy::No);
        stmt.BindInt64(7, kL);
        stmt.BindDateTime(8, kDt);
        stmt.BindBoolean(9, kB);
        ASSERT_EQ(BE_SQLITE_DONE, stmt.Step());
    }

    const auto sql =
        "SELECT                                        "
        "   EXTRACT_INST(ecClassId, ecInstanceId),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 's'  ),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'i'  ),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'd'  ),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'p2d'),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'p3d'),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'bi' ),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'l'  ),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'dt' ),"
        "   EXTRACT_PROP(ecClassId, ecInstanceId, 'b'  ) "
        "FROM ts.P                                     ";

    ECSqlStatement stmt;
    ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, sql));
    ASSERT_EQ(stmt.Step(), BE_SQLITE_ROW);

    int i = 1;
    ASSERT_STRCASEEQ(stmt.GetValueText(i++), kText);
    ASSERT_EQ(stmt.GetValueInt(i++), kI);
    ASSERT_EQ(stmt.GetValueDouble(i++), kD);
    ASSERT_STRCASEEQ(stmt.GetValueText(i++), "{\"X\":2.0,\"Y\":4.0}"); // return as json
    ASSERT_STRCASEEQ(stmt.GetValueText(i++), "{\"X\":4.0,\"Y\":5.0,\"Z\":6.0}"); // return as json
    ASSERT_EQ(memcmp(stmt.GetValueBlob(i++), &kBi[0], kBiLen), 0);
    ASSERT_EQ(stmt.GetValueInt64(i++), kL);
    ASSERT_TRUE(stmt.GetValueDateTime (i++).Equals(kDt, true));
    ASSERT_EQ(stmt.GetValueBoolean(i++), kB);

    if ("use syntax to extract property") {
        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, "SELECT $->s, $->i, $->d, $->p2d, $->p3d, $->bi, $->l, $->dt, $->b FROM ts.P"));
        const auto expectedSQL = "SELECT extract_prop([P].[ECClassId],[P].[ECInstanceId],'s'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'i'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'d'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'p2d'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'p3d'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'bi'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'l'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'dt'),extract_prop([P].[ECClassId],[P].[ECInstanceId],'b') FROM (SELECT [Id] ECInstanceId,73 ECClassId FROM [main].[ts_P]) [P]";
        EXPECT_STRCASEEQ(expectedSQL, stmt.GetNativeSql());
        if(stmt.Step() == BE_SQLITE_ROW) {
            int i = 0;
            ASSERT_STRCASEEQ(stmt.GetValueText(i++), kText);
            ASSERT_EQ(stmt.GetValueInt(i++), kI);
            ASSERT_EQ(stmt.GetValueDouble(i++), kD);
            ASSERT_STRCASEEQ(stmt.GetValueText(i++), "{\"X\":2.0,\"Y\":4.0}"); // return as json
            ASSERT_STRCASEEQ(stmt.GetValueText(i++), "{\"X\":4.0,\"Y\":5.0,\"Z\":6.0}"); // return as json
            ASSERT_EQ(memcmp(stmt.GetValueBlob(i++), &kBi[0], kBiLen), 0);
            ASSERT_EQ(stmt.GetValueInt64(i++), kL);
            ASSERT_TRUE(stmt.GetValueDateTime (i++).Equals(kDt, true));
            ASSERT_EQ(stmt.GetValueBoolean(i++), kB);
        }
    }

}

//---------------------------------------------------------------------------------------
// @bsimethod
//+---------------+---------------+---------------+---------------+---------------+------
TEST_F(InstanceReaderFixture, prop_exists) {
    ASSERT_EQ(SUCCESS, SetupECDb("PROP_EXISTS.ecdb", SchemaItem(
        R"xml(<ECSchema schemaName='TestSchema' alias='ts' version='1.0' xmlns='http://www.bentley.com/schemas/Bentley.ECXML.3.1'>
                   <ECSchemaReference name="ECDbMap" version="02.00" alias="ecdbmap" />
                   <ECEntityClass typeName="Base" modifier="Abstract">
                        <ECCustomAttributes>
                            <ClassMap xmlns="ECDbMap.02.00">
                                <MapStrategy>TablePerHierarchy</MapStrategy>
                            </ClassMap>
                            <JoinedTablePerDirectSubclass xmlns="ECDbMap.02.00"/>
                        </ECCustomAttributes>
                        <ECProperty propertyName="Prop1" typeName="string" />
                        <ECProperty propertyName="Prop2" typeName="int" />
                    </ECEntityClass>
                    <ECEntityClass typeName="Sub">
                      <BaseClass>Base</BaseClass>
                      <ECProperty propertyName="SubProp1" typeName="string" />
                      <ECProperty propertyName="SubProp2" typeName="int" />
                     </ECEntityClass>
               </ECSchema>)xml")));
    m_ecdb.SaveChanges();

    if ("case sensitive check") {
        const auto sql =
            "SELECT "
            "   PROP_EXISTS(ec_classid('ts.Base'), 'ECClassId'   ),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'ECInstanceId'),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'Prop1'       ),"
            "   PROP_EXISTS(ec_classId('ts.base'), 'Prop2'       ),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'SubProp1'    ),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'SubProp2'    ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'ECClassId'   ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'ECInstanceId'),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'Prop1'       ),"
            "   PROP_EXISTS(ec_classId('ts.Sub'),  'Prop2'       ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'SubProp1'    ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'SubProp2'    )";

        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, sql));

        ASSERT_EQ(stmt.Step(), BE_SQLITE_ROW);

        int i = 0;
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 0); // SubProp1 does not exist in base
        ASSERT_EQ(stmt.GetValueInt(i++), 0); // SubProp2 does not exist in base
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
    }

    if ("case insensitive check") {
        const auto sql =
            "SELECT "
            "   PROP_EXISTS(ec_classid('ts.Base'), 'ECCLASSID'   ),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'ECINSTANCEID'),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'PROP1'       ),"
            "   PROP_EXISTS(ec_classId('ts.base'), 'PROP2'       ),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'SUBPROP1'    ),"
            "   PROP_EXISTS(ec_classid('ts.Base'), 'SUBPROP2'    ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'ECCLASSID'   ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'ECINSTANCEID'),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'PROP1'       ),"
            "   PROP_EXISTS(ec_classId('ts.Sub'),  'PROP2'       ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'SUBPROP1'    ),"
            "   PROP_EXISTS(ec_classid('ts.Sub'),  'SUBPROP2'    )";

        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, sql));
        ASSERT_EQ(stmt.Step(), BE_SQLITE_ROW);

        int i = 0;
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 0); // SubProp1 does not exist in base
        ASSERT_EQ(stmt.GetValueInt(i++), 0); // SubProp2 does not exist in base
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
        ASSERT_EQ(stmt.GetValueInt(i++), 1);
    }
}

//---------------------------------------------------------------------------------------
// @bsimethod
//+---------------+---------------+---------------+---------------+---------------+------
TEST_F(InstanceReaderFixture, nested_struct) {
    ASSERT_EQ(SUCCESS, SetupECDb("nested_struct.ecdb", SchemaItem(
        R"xml(<ECSchema schemaName="TestSchema" alias="ts" version="1.0" xmlns="http://www.bentley.com/schemas/Bentley.ECXML.3.1">
                <ECSchemaReference name="ECDbMap" version="02.00" alias="ecdbmap" />
                <ECSchemaReference name="CoreCustomAttributes" version="01.00.00" alias="CoreCA" />
                <ECStructClass typeName="struct_p" description="Struct with primitive props (default mappings)">
                    <ECProperty propertyName="b" typeName="boolean" />
                    <ECProperty propertyName="bi" typeName="binary" />
                    <ECProperty propertyName="d" typeName="double" />
                    <ECProperty propertyName="dt" typeName="dateTime" />
                    <ECProperty propertyName="dtUtc" typeName="dateTime">
                        <ECCustomAttributes>
                            <DateTimeInfo xmlns="CoreCustomAttributes.01.00.00">
                                <DateTimeKind>Utc</DateTimeKind>
                            </DateTimeInfo>
                        </ECCustomAttributes>
                    </ECProperty>
                    <ECProperty propertyName="i" typeName="int" />
                    <ECProperty propertyName="l" typeName="long" />
                    <ECProperty propertyName="s" typeName="string" />
                    <ECProperty propertyName="p2d" typeName="point2d" />
                    <ECProperty propertyName="p3d" typeName="point3d" />
                    <ECProperty propertyName="geom" typeName="Bentley.Geometry.Common.IGeometry" />
                </ECStructClass>
                <ECStructClass typeName="struct_pa" description="Primitive array">
                    <ECArrayProperty propertyName="b_array" typeName="boolean" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="bi_array" typeName="binary" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="d_array" typeName="double" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="dt_array" typeName="dateTime" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="dtUtc_array" typeName="dateTime" minOccurs="0" maxOccurs="unbounded">
                        <ECCustomAttributes>
                            <DateTimeInfo xmlns="CoreCustomAttributes.01.00.00">
                                <DateTimeKind>Utc</DateTimeKind>
                            </DateTimeInfo>
                        </ECCustomAttributes>
                    </ECArrayProperty>
                    <ECArrayProperty propertyName="i_array" typeName="int" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="l_array" typeName="long" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="s_array" typeName="string" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="p2d_array" typeName="point2d" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="p3d_array" typeName="point3d" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="geom_array" typeName="Bentley.Geometry.Common.IGeometry" minOccurs="0" maxOccurs="unbounded" />
                </ECStructClass>
                <ECEntityClass typeName="e_mix"
                    description="Cover all primitive, primitive array, struct of primitive, array of struct">
                    <ECCustomAttributes>
                        <ClassMap xmlns="ECDbMap.02.00">
                            <MapStrategy>TablePerHierarchy</MapStrategy>
                        </ClassMap>
                        <ShareColumns xmlns="ECDbMap.02.00">
                            <MaxSharedColumnsBeforeOverflow>500</MaxSharedColumnsBeforeOverflow>
                            <ApplyToSubclassesOnly>False</ApplyToSubclassesOnly>
                        </ShareColumns>
                    </ECCustomAttributes>
                    <ECNavigationProperty propertyName="parent" relationshipName="e_mix_has_base_mix" direction="Backward">
                        <ECCustomAttributes>
                            <ForeignKeyConstraint xmlns="ECDbMap.02.00.00">
                                <OnDeleteAction>Cascade</OnDeleteAction>
                                <OnUpdateAction>Cascade</OnUpdateAction>
                            </ForeignKeyConstraint>
                        </ECCustomAttributes>
                    </ECNavigationProperty>
                    <ECProperty propertyName="b" typeName="boolean" />
                    <ECProperty propertyName="bi" typeName="binary" />
                    <ECProperty propertyName="d" typeName="double" />
                    <ECProperty propertyName="dt" typeName="dateTime" />
                    <ECProperty propertyName="dtUtc" typeName="dateTime">
                        <ECCustomAttributes>
                            <DateTimeInfo xmlns="CoreCustomAttributes.01.00.00">
                                <DateTimeKind>Utc</DateTimeKind>
                            </DateTimeInfo>
                        </ECCustomAttributes>
                    </ECProperty>
                    <ECProperty propertyName="i" typeName="int" />
                    <ECProperty propertyName="l" typeName="long" />
                    <ECProperty propertyName="s" typeName="string" />
                    <ECProperty propertyName="p2d" typeName="point2d" />
                    <ECProperty propertyName="p3d" typeName="point3d" />
                    <ECProperty propertyName="geom" typeName="Bentley.Geometry.Common.IGeometry" />
                    <ECArrayProperty propertyName="b_array" typeName="boolean" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="bi_array" typeName="binary" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="d_array" typeName="double" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="dt_array" typeName="dateTime" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="dtUtc_array" typeName="dateTime" minOccurs="0"
                        maxOccurs="unbounded">
                        <ECCustomAttributes>
                            <DateTimeInfo xmlns="CoreCustomAttributes.01.00.00">
                                <DateTimeKind>Utc</DateTimeKind>
                            </DateTimeInfo>
                        </ECCustomAttributes>
                    </ECArrayProperty>
                    <ECArrayProperty propertyName="i_array" typeName="int" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="l_array" typeName="long" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="s_array" typeName="string" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="p2d_array" typeName="point2d" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="p3d_array" typeName="point3d" minOccurs="0" maxOccurs="unbounded" />
                    <ECArrayProperty propertyName="geom_array" typeName="Bentley.Geometry.Common.IGeometry" minOccurs="0" maxOccurs="unbounded" />
                    <ECStructProperty propertyName="p" typeName="struct_p" />
                    <ECStructProperty propertyName="pa" typeName="struct_pa" />
                    <ECStructArrayProperty propertyName="array_of_p" typeName="struct_p" minOccurs="0" maxOccurs="unbounded" />
                    <ECStructArrayProperty propertyName="array_of_pa" typeName="struct_pa" minOccurs="0" maxOccurs="unbounded" />
                </ECEntityClass>
                <ECRelationshipClass typeName="e_mix_has_base_mix" strength="Embedding" modifier="Sealed">
                    <Source multiplicity="(0..1)" polymorphic="false" roleLabel="e_mix">
                        <Class class="e_mix" />
                    </Source>
                    <Target multiplicity="(0..*)" polymorphic="false" roleLabel="e_mix">
                        <Class class="e_mix" />
                    </Target>
                </ECRelationshipClass>
            </ECSchema>)xml")));
    m_ecdb.Schemas().CreateClassViewsInDb();
    m_ecdb.SaveChanges();
    ECInstanceKey instKey;
    if ("insert data") {
        ECSqlStatement stmt;
        auto rc = stmt.Prepare(m_ecdb, R"sql(
            insert into ts.e_mix(
                parent,
                b, bi, d, dt, dtUtc, i, l, s, p2d, p3d, geom,
                b_array, bi_array, d_array, dt_array, dtUtc_array,
                i_array, l_array, s_array, p2d_array, p3d_array, geom_array,
                p, pa, array_of_p, array_of_pa)
            values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
        )sql");

        const auto b = true;
        const uint8_t bi[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21};
        const double d = PI;
        const auto dt = DateTime(DateTime::Kind::Unspecified, 2017, 1, 17, 0, 0);
        const auto dtUtc = DateTime(DateTime::Kind::Utc, 2018, 2, 17, 0, 0);
        const int i = 0xfffafaff;
        const int64_t l = 0xfffafaffafffffff;
        const auto s = std::string{"Hello, World!"};
        const auto p2d = DPoint2d::From(22.33, -21.34);
        const auto p3d = DPoint3d::From(12.13, -42.34, -93.12);
        auto geom = IGeometry::Create(ICurvePrimitive::CreateLine(DSegment3d::From(0.0, 0.0, 0.0, 1.0, 1.0, 1.0)));
        const bool b_array[] = {true, false, true};
        const std::vector<std::vector<uint8_t>> bi_array = {
            {0x48, 0x65, 0x6},
            {0x48, 0x65, 0x6},
            {0x48, 0x65, 0x6}
        };
        const std::vector<double> d_array = {123.3434, 345.223, -532.123};
        const std::vector<DateTime> dt_array = {
            DateTime(DateTime::Kind::Unspecified, 2017, 1, 14, 0, 0),
            DateTime(DateTime::Kind::Unspecified, 2018, 1, 13, 0, 0),
            DateTime(DateTime::Kind::Unspecified, 2019, 1, 11, 0, 0),
        };
        const std::vector<DateTime> dtUtc_array = {
            DateTime(DateTime::Kind::Utc, 2017, 1, 17, 0, 0),
            DateTime(DateTime::Kind::Utc, 2018, 1, 11, 0, 0),
            DateTime(DateTime::Kind::Utc, 2019, 1, 10, 0, 0),
        };
        const std::vector<int> i_array = {3842, -4923, 8291};
        const std::vector<int64_t> l_array = {384242, -234923, 528291};
        const std::vector<std::string> s_array = {"Bentley", "System"};
        const std::vector<DPoint2d> p2d_array = {
            DPoint2d::From(22.33 , -81.17),
            DPoint2d::From(-42.74,  16.29),
            DPoint2d::From(77.45 , -32.98),
        };
        const std::vector<DPoint3d> p3d_array = {
            DPoint3d::From( 84.13,  99.23, -121.75),
            DPoint3d::From(-90.34,  45.75, -452.34),
            DPoint3d::From(-12.54, -84.23, -343.45),
        };
        const std::vector<IGeometryPtr> geom_array = {
            IGeometry::Create(ICurvePrimitive::CreateLine(DSegment3d::From(0.0, 0.0, 0.0, 4.0, 2.1, 1.2))),
            IGeometry::Create(ICurvePrimitive::CreateLine(DSegment3d::From(0.0, 0.0, 0.0, 1.1, 2.5, 4.2))),
            IGeometry::Create(ICurvePrimitive::CreateLine(DSegment3d::From(0.0, 0.0, 0.0, 9.1, 3.6, 3.8))),
        };
        int idx = 0;
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindNull(++idx));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindBoolean(++idx, true));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindBlob(++idx, (void const*)bi, (int)sizeof(bi), IECSqlBinder::MakeCopy::No));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindDouble(++idx, d));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindDateTime(++idx, dt));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindDateTime(++idx, dtUtc));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindInt(++idx, i));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindInt64(++idx, l));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindText(++idx, s.c_str(), IECSqlBinder::MakeCopy::No));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindPoint2d(++idx, p2d));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindPoint3d(++idx, p3d));
        ASSERT_EQ(ECSqlStatus::Success, stmt.BindGeometry(++idx, *geom));
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : b_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindBoolean(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : bi_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindBlob((void const*)&m[0], (int)m.size(), IECSqlBinder::MakeCopy::No));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : d_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindDouble(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : dt_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindDateTime(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : dtUtc_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindDateTime(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : i_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindInt(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : l_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindInt64(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : s_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindText(m.c_str(), IECSqlBinder::MakeCopy::No));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : p2d_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindPoint2d(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : p3d_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindPoint3d(m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : geom_array)
                ASSERT_EQ(ECSqlStatus::Success, v->AddArrayElement().BindGeometry(*m));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            ASSERT_EQ(ECSqlStatus::Success, (*v)["b"].BindBoolean(b));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["bi"].BindBlob((void const*)bi, (int)sizeof(bi), IECSqlBinder::MakeCopy::No));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["d"].BindDouble(d));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["dt"].BindDateTime(dt));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["dtUtc"].BindDateTime(dtUtc));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["i"].BindInt(i));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["l"].BindInt64(l));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["s"].BindText(s.c_str(), IECSqlBinder::MakeCopy::No));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["p2d"].BindPoint2d(p2d));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["p3d"].BindPoint3d(p3d));
            ASSERT_EQ(ECSqlStatus::Success, (*v)["geom"].BindGeometry(*geom));
        }
        if (auto v = &stmt.GetBinder(++idx)) {
            for(auto& m : b_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["b_array"].AddArrayElement().BindBoolean(m));
            for(auto& m : bi_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["bi_array"].AddArrayElement().BindBlob((void const*)&m[0], (int)m.size(), IECSqlBinder::MakeCopy::No));
            for(auto& m : d_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["d_array"].AddArrayElement().BindDouble(m));
            for(auto& m : dt_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["dt_array"].AddArrayElement().BindDateTime(m));
            for(auto& m : dtUtc_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["dtUtc_array"].AddArrayElement().BindDateTime(m));
            for(auto& m : i_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["i_array"].AddArrayElement().BindInt(m));
            for(auto& m : l_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["l_array"].AddArrayElement().BindInt64(m));
            for(auto& m : s_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["s_array"].AddArrayElement().BindText(m.c_str(), IECSqlBinder::MakeCopy::No));
            for(auto& m : p2d_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["p2d_array"].AddArrayElement().BindPoint2d(m));
            for(auto& m : p3d_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["p3d_array"].AddArrayElement().BindPoint3d(m));
            for(auto& m : geom_array)
                ASSERT_EQ(ECSqlStatus::Success, (*v)["geom_array"].AddArrayElement().BindGeometry(*m));
        }
        if (auto o = &stmt.GetBinder(++idx)) {
            for (int n = 0; n < 2;++n) {
                auto& v = o->AddArrayElement();
                ASSERT_EQ(ECSqlStatus::Success, v["b"].BindBoolean(b_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["bi"].BindBlob((void const*)&bi_array[n][0], (int)bi_array[n].size(), IECSqlBinder::MakeCopy::No));
                ASSERT_EQ(ECSqlStatus::Success, v["d"].BindDouble(d_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["dt"].BindDateTime(dt_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["dtUtc"].BindDateTime(dtUtc_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["i"].BindInt(i_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["l"].BindInt64(l_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["s"].BindText(s_array[n].c_str(), IECSqlBinder::MakeCopy::No));
                ASSERT_EQ(ECSqlStatus::Success, v["p2d"].BindPoint2d(p2d_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["p3d"].BindPoint3d(p3d_array[n]));
                ASSERT_EQ(ECSqlStatus::Success, v["geom"].BindGeometry(*geom_array[n]));
            }
        }
        if (auto o = &stmt.GetBinder(++idx)) {
            for (int n = 0; n < 2;++n) {
                auto& v = o->AddArrayElement();
                for(auto& m : b_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["b_array"].AddArrayElement().BindBoolean(m));
                for(auto& m : bi_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["bi_array"].AddArrayElement().BindBlob((void const*)&m[0], (int)m.size(), IECSqlBinder::MakeCopy::No));
                for(auto& m : d_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["d_array"].AddArrayElement().BindDouble(m));
                for(auto& m : dt_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["dt_array"].AddArrayElement().BindDateTime(m));
                for(auto& m : dtUtc_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["dtUtc_array"].AddArrayElement().BindDateTime(m));
                for(auto& m : i_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["i_array"].AddArrayElement().BindInt(m));
                for(auto& m : l_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["l_array"].AddArrayElement().BindInt64(m));
                for(auto& m : s_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["s_array"].AddArrayElement().BindText(m.c_str(), IECSqlBinder::MakeCopy::No));
                for(auto& m : p2d_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["p2d_array"].AddArrayElement().BindPoint2d(m));
                for(auto& m : p3d_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["p3d_array"].AddArrayElement().BindPoint3d(m));
                for(auto& m : geom_array)
                    ASSERT_EQ(ECSqlStatus::Success, v["geom_array"].AddArrayElement().BindGeometry(*m));
            }
        }
        ASSERT_EQ(BE_SQLITE_DONE, stmt.Step(instKey));
        m_ecdb.SaveChanges();
    }

    BeJsDocument expected;
    expected.Parse(R"json({
        "ECInstanceId": "0x1",
        "ECClassId": "TestSchema.e_mix",
        "b": true,
        "bi": "encoding=base64;SA==",
        "d": 3.141592653589793,
        "dt": "2017-01-17T00:00:00.000",
        "dtUtc": "2018-02-17T00:00:00.000Z",
        "i": -328961,
        "l": -1412873783869441.0,
        "s": "Hello, World!",
        "p2d": {
            "X": 22.33,
            "Y": -21.34
        },
        "p3d": {
            "X": 12.13,
            "Y": -42.34,
            "Z": -93.12
        },
        "geom": {
            "lineSegment": [
                [
                    0,
                    0,
                    0
                ],
                [
                    1,
                    1,
                    1
                ]
            ]
        },
        "b_array": [
            true,
            false,
            true
        ],
        "bi_array": [
            "encoding=base64;SA==",
            "encoding=base64;SA==",
            "encoding=base64;SA=="
        ],
        "d_array": [
            123.3434,
            345.223,
            -532.123
        ],
        "dt_array": [
            "2017-01-14T00:00:00.000",
            "2018-01-13T00:00:00.000",
            "2019-01-11T00:00:00.000"
        ],
        "dtUtc_array": [
            "2017-01-17T00:00:00.000",
            "2018-01-11T00:00:00.000",
            "2019-01-10T00:00:00.000"
        ],
        "i_array": [
            3842,
            -4923,
            8291
        ],
        "l_array": [
            384242.0,
            -234923.0,
            528291.0
        ],
        "s_array": [
            "Bentley",
            "System"
        ],
        "p2d_array": [
            {
                "X": 22.33,
                "Y": -81.17
            },
            {
                "X": -42.74,
                "Y": 16.29
            },
            {
                "X": 77.45,
                "Y": -32.98
            }
        ],
        "p3d_array": [
            {
                "X": 84.13,
                "Y": 99.23,
                "Z": -121.75
            },
            {
                "X": -90.34,
                "Y": 45.75,
                "Z": -452.34
            },
            {
                "X": -12.54,
                "Y": -84.23,
                "Z": -343.45
            }
        ],
        "geom_array": [
            {
                "lineSegment": [
                    [
                    0,
                    0,
                    0
                    ],
                    [
                    4,
                    2.1,
                    1.2
                    ]
                ]
            },
            {
                "lineSegment": [
                    [
                    0,
                    0,
                    0
                    ],
                    [
                    1.1,
                    2.5,
                    4.2
                    ]
                ]
            },
            {
                "lineSegment": [
                    [
                    0,
                    0,
                    0
                    ],
                    [
                    9.1,
                    3.6,
                    3.8
                    ]
                ]
            }
        ],
        "p": {
            "b": true,
            "bi": "encoding=base64;SA==",
            "d": 3.141592653589793,
            "dt": "2017-01-17T00:00:00.000",
            "dtUtc": "2018-02-17T00:00:00.000Z",
            "geom": {
                "lineSegment": [
                    [
                    0,
                    0,
                    0
                    ],
                    [
                    1,
                    1,
                    1
                    ]
                ]
            },
            "i": -328961,
            "l": -1412873783869441.0,
            "p2d": {
                "X": 22.33,
                "Y": -21.34
            },
            "p3d": {
                "X": 12.13,
                "Y": -42.34,
                "Z": -93.12
            },
            "s": "Hello, World!"
        },
        "pa": {
            "b_array": [
                true,
                false,
                true
            ],
            "bi_array": [
                "encoding=base64;SA==",
                "encoding=base64;SA==",
                "encoding=base64;SA=="
            ],
            "d_array": [
                123.3434,
                345.223,
                -532.123
            ],
            "dt_array": [
                "2017-01-14T00:00:00.000",
                "2018-01-13T00:00:00.000",
                "2019-01-11T00:00:00.000"
            ],
            "dtUtc_array": [
                "2017-01-17T00:00:00.000",
                "2018-01-11T00:00:00.000",
                "2019-01-10T00:00:00.000"
            ],
            "geom_array": [
                {
                    "lineSegment": [
                    [
                        0,
                        0,
                        0
                    ],
                    [
                        4,
                        2.1,
                        1.2
                    ]
                    ]
                },
                {
                    "lineSegment": [
                    [
                        0,
                        0,
                        0
                    ],
                    [
                        1.1,
                        2.5,
                        4.2
                    ]
                    ]
                },
                {
                    "lineSegment": [
                    [
                        0,
                        0,
                        0
                    ],
                    [
                        9.1,
                        3.6,
                        3.8
                    ]
                    ]
                }
            ],
            "i_array": [
                3842,
                -4923,
                8291
            ],
            "l_array": [
                384242.0,
                -234923.0,
                528291.0
            ],
            "p2d_array": [
                {
                    "X": 22.33,
                    "Y": -81.17
                },
                {
                    "X": -42.74,
                    "Y": 16.29
                },
                {
                    "X": 77.45,
                    "Y": -32.98
                }
            ],
            "p3d_array": [
                {
                    "X": 84.13,
                    "Y": 99.23,
                    "Z": -121.75
                },
                {
                    "X": -90.34,
                    "Y": 45.75,
                    "Z": -452.34
                },
                {
                    "X": -12.54,
                    "Y": -84.23,
                    "Z": -343.45
                }
            ],
            "s_array": [
                "Bentley",
                "System"
            ]
        },
        "array_of_p": [
            {
                "b": true,
                "bi": "encoding=base64;SA==",
                "d": 123.3434,
                "dt": "2017-01-14T00:00:00.000",
                "dtUtc": "2017-01-17T00:00:00.000Z",
                "i": 3842,
                "l": 384242.0,
                "s": "Bentley",
                "p2d": {
                    "X": 22.33,
                    "Y": -81.17
                },
                "p3d": {
                    "X": 84.13,
                    "Y": 99.23,
                    "Z": -121.75
                },
                "geom": {
                    "lineSegment": [
                    [
                        0,
                        0,
                        0
                    ],
                    [
                        4,
                        2.1,
                        1.2
                    ]
                    ]
                }
            },
            {
                "b": false,
                "bi": "encoding=base64;SA==",
                "d": 345.223,
                "dt": "2018-01-13T00:00:00.000",
                "dtUtc": "2018-01-11T00:00:00.000Z",
                "i": -4923,
                "l": -234923.0,
                "s": "System",
                "p2d": {
                    "X": -42.74,
                    "Y": 16.29
                },
                "p3d": {
                    "X": -90.34,
                    "Y": 45.75,
                    "Z": -452.34
                },
                "geom": {
                    "lineSegment": [
                    [
                        0,
                        0,
                        0
                    ],
                    [
                        1.1,
                        2.5,
                        4.2
                    ]
                    ]
                }
            }
        ],
        "array_of_pa": [
            {
                "b_array": [
                    true,
                    false,
                    true
                ],
                "bi_array": [
                    "encoding=base64;SA==",
                    "encoding=base64;SA==",
                    "encoding=base64;SA=="
                ],
                "d_array": [
                    123.3434,
                    345.223,
                    -532.123
                ],
                "dt_array": [
                    "2017-01-14T00:00:00.000",
                    "2018-01-13T00:00:00.000",
                    "2019-01-11T00:00:00.000"
                ],
                "dtUtc_array": [
                    "2017-01-17T00:00:00.000Z",
                    "2018-01-11T00:00:00.000Z",
                    "2019-01-10T00:00:00.000Z"
                ],
                "i_array": [
                    3842,
                    -4923,
                    8291
                ],
                "l_array": [
                    384242.0,
                    -234923.0,
                    528291.0
                ],
                "s_array": [
                    "Bentley",
                    "System"
                ],
                "p2d_array": [
                    {
                    "X": 22.33,
                    "Y": -81.17
                    },
                    {
                    "X": -42.74,
                    "Y": 16.29
                    },
                    {
                    "X": 77.45,
                    "Y": -32.98
                    }
                ],
                "p3d_array": [
                    {
                    "X": 84.13,
                    "Y": 99.23,
                    "Z": -121.75
                    },
                    {
                    "X": -90.34,
                    "Y": 45.75,
                    "Z": -452.34
                    },
                    {
                    "X": -12.54,
                    "Y": -84.23,
                    "Z": -343.45
                    }
                ],
                "geom_array": [
                    {
                    "lineSegment": [
                        [
                            0,
                            0,
                            0
                        ],
                        [
                            4,
                            2.1,
                            1.2
                        ]
                    ]
                    },
                    {
                    "lineSegment": [
                        [
                            0,
                            0,
                            0
                        ],
                        [
                            1.1,
                            2.5,
                            4.2
                        ]
                    ]
                    },
                    {
                    "lineSegment": [
                        [
                            0,
                            0,
                            0
                        ],
                        [
                            9.1,
                            3.6,
                            3.8
                        ]
                    ]
                    }
                ]
            },
            {
                "b_array": [
                    true,
                    false,
                    true
                ],
                "bi_array": [
                    "encoding=base64;SA==",
                    "encoding=base64;SA==",
                    "encoding=base64;SA=="
                ],
                "d_array": [
                    123.3434,
                    345.223,
                    -532.123
                ],
                "dt_array": [
                    "2017-01-14T00:00:00.000",
                    "2018-01-13T00:00:00.000",
                    "2019-01-11T00:00:00.000"
                ],
                "dtUtc_array": [
                    "2017-01-17T00:00:00.000Z",
                    "2018-01-11T00:00:00.000Z",
                    "2019-01-10T00:00:00.000Z"
                ],
                "i_array": [
                    3842,
                    -4923,
                    8291
                ],
                "l_array": [
                    384242.0,
                    -234923.0,
                    528291.0
                ],
                "s_array": [
                    "Bentley",
                    "System"
                ],
                "p2d_array": [
                    {
                    "X": 22.33,
                    "Y": -81.17
                    },
                    {
                    "X": -42.74,
                    "Y": 16.29
                    },
                    {
                    "X": 77.45,
                    "Y": -32.98
                    }
                ],
                "p3d_array": [
                    {
                    "X": 84.13,
                    "Y": 99.23,
                    "Z": -121.75
                    },
                    {
                    "X": -90.34,
                    "Y": 45.75,
                    "Z": -452.34
                    },
                    {
                    "X": -12.54,
                    "Y": -84.23,
                    "Z": -343.45
                    }
                ],
                "geom_array": [
                    {
                    "lineSegment": [
                        [
                            0,
                            0,
                            0
                        ],
                        [
                            4,
                            2.1,
                            1.2
                        ]
                    ]
                    },
                    {
                    "lineSegment": [
                        [
                            0,
                            0,
                            0
                        ],
                        [
                            1.1,
                            2.5,
                            4.2
                        ]
                    ]
                    },
                    {
                    "lineSegment": [
                        [
                            0,
                            0,
                            0
                        ],
                        [
                            9.1,
                            3.6,
                            3.8
                        ]
                    ]
                    }
                ]
            }
        ]
        })json");

    if ("check out instance reader with complex data") {
        auto& reader = m_ecdb.GetInstanceReader();
        auto pos = InstanceReader::Position(instKey.GetInstanceId(), instKey.GetClassId());
        ASSERT_EQ(true, reader.Seek(pos, [&](InstanceReader::IRowContext const& row) {
            EXPECT_STRCASEEQ(expected.Stringify(StringifyFormat::Indented).c_str(), row.GetJson().Stringify(StringifyFormat::Indented).c_str());
        }));
    }
    if ("test if we get similar result from instance rendered by $") {
        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, "SELECT $ FROM ts.e_mix"));
        ASSERT_EQ(stmt.Step(), BE_SQLITE_ROW);
        BeJsDocument actual;
        actual.Parse(stmt.GetValueText(0));
        EXPECT_STRCASEEQ(expected.Stringify(StringifyFormat::Indented).c_str(), actual.Stringify(StringifyFormat::Indented).c_str());
    }

    if ("test if we get similar result from instance rendered by $") {
        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(m_ecdb, "SELECT $ FROM ts.e_mix"));
        ASSERT_EQ(stmt.Step(), BE_SQLITE_ROW);
        BeJsDocument actual;
        actual.Parse(stmt.GetValueText(0));
        EXPECT_STRCASEEQ(expected.Stringify(StringifyFormat::Indented).c_str(), actual.Stringify(StringifyFormat::Indented).c_str());
    }
        if ("test if we get similar result from instance rendered by $") {
        ECSqlStatement stmt;
        ASSERT_EQ(ECSqlStatus::Success, stmt.Prepare(
            m_ecdb, R"sql(
                SELECT
                    $->b,
                    $->bi,
                    $->d,
                    $->dt,
                    $->dtUtc,
                    $->i,
                    $->l,
                    $->s,
                    $->p2d,
                    $->p3d,
                    $->b_array,
                    $->bi_array,
                    $->d_array,
                    $->dt_array,
                    $->dtUtc_array,
                    $->i_array,
                    $->l_array,
                    $->s_array,
                    $->p2d_array,
                    $->p3d_array,
                    $->geom_array,
                    $->p,
                    $->pa,
                    $->array_of_p,
                    $->array_of_pa
                FROM ts.e_mix
            )sql"));
        ASSERT_EQ(stmt.Step(), BE_SQLITE_ROW);
        ASSERT_EQ(expected["b"].asBool(), stmt.GetValueBoolean(0));
        ASSERT_STRCASEEQ("Hello, World!", stmt.GetValueText(1));
        ASSERT_EQ(expected["d"].asDouble(), stmt.GetValueDouble(2));
        /*
            ASSERT_STRCASEEQ(expected["dt"].asCString(), stmt.GetValueDateTime(3).ToString().c_str());
            ASSERT_STRCASEEQ(expected["dtUtc"].asCString(), stmt.GetValueDateTime(4).ToString().c_str());
            ASSERT_EQ(expected["i"].asInt(), stmt.GetValueInt64(4));
            ASSERT_EQ(expected["l"].asInt64(), stmt.GetValueInt64(5));
            ASSERT_STRCASEEQ(expected["s"].asCString(), stmt.GetValueText(6));
            ASSERT_STRCASEEQ(expected["p2d"].Stringify().c_str(), stmt.GetValueText(7));
            ASSERT_STRCASEEQ(expected["p3d"].Stringify().c_str(), stmt.GetValueText(8));
            ASSERT_STRCASEEQ(expected["b_array"].Stringify().c_str(), stmt.GetValueText(9));
            ASSERT_STRCASEEQ(expected["bi_array"].Stringify().c_str(), stmt.GetValueText(10));
        */

        // BeJsDocument actual;
        // actual.Parse(stmt.GetValueText(0));
        // EXPECT_STRCASEEQ(expected.Stringify(StringifyFormat::Indented).c_str(), actual.Stringify(StringifyFormat::Indented).c_str());
    }
}

END_ECDBUNITTESTS_NAMESPACE