////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "CapabilityCompletion.h"
#include "LSPErrorCodes.h"
#include "MessageWriter.h"
#include "LenientXamlParser.h"
#include "LangServerReflectionHelper.h"

#include <NsCore/String.h>
#include <NsCore/Factory.h>
#include <NsCore/Nullable.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/UIElement.h>
#include <NsGui/ContentPropertyMetaData.h>
#include <NsGui/DependencyData.h>
#include <NsGui/Brush.h>
#include <NsGui/ControlTemplate.h>
#include <NsGui/DataTemplate.h>
#include <NsGui/ResourceDictionary.h>
#include <NsGui/Binding.h>
#include <NsGui/DynamicResourceExtension.h>
#include <NsGui/StaticResourceExtension.h>
#include <NsGui/UICollection.h>
#include <NsDrawing/Color.h>
#include <NsApp/LangServer.h>

#include <cctype>


#ifdef NS_LANG_SERVER_ENABLED

namespace
{
struct SnippetData
{
    enum Filter
    {
        Filter_None,
        Filter_Root,
        Filter_ResourceDictionary,
        Filter_ControlTemplate
    };

    Filter filter;
    Noesis::String detail;
    Noesis::String text;
    Noesis::Symbol typeId;

    SnippetData() : filter()
    {
    }

    SnippetData(const char* _detail, Filter _filter, const char* _text)
        : filter(_filter), detail(_detail), text(_text)
    {
    }

    SnippetData(const char* _detail, Filter _filter, const char* _text, const char* _type)
        : filter(_filter), detail(_detail), text(_text),
        typeId(_type, Noesis::Symbol::NullIfNotFound())
    {
    }
};

struct CompletionItemData
{
    uint32_t itemKind;
    Noesis::String detail;
    SnippetData snippet;

    explicit CompletionItemData(const uint32_t _itemKind) : itemKind(_itemKind), snippet()
    {
    }

    CompletionItemData(const uint32_t _itemKind, const char* _detail, const SnippetData& _snippet)
        : itemKind(_itemKind), detail(_detail), snippet(_snippet)
    {
    }
};

typedef Noesis::HashMap<Noesis::String, CompletionItemData> ItemSet;
typedef Noesis::HashMap<Noesis::String, SnippetData> SnippetSet;

const uint32_t ItemKindClass = 7;
const uint32_t ItemKindProperty = 10;
const uint32_t ItemKindValue = 12;
const uint32_t ItemKindSnippet = 15;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void PopulateNodeSnippets(SnippetSet& set)
{

    set.Insert("button", SnippetData("Button", SnippetData::Filter_None, "<Button Width=\"120\" Height=\"50\" Content=\"${1:Button}\" Margin=\"5\" />$0"));
    set.Insert("check", SnippetData("CheckBox", SnippetData::Filter_None, "<CheckBox Width=\"120\" Height=\"50\" Content=\"${1:CheckBox}\" IsChecked=\"${2:True}\" Margin=\"5\" />$3"));
    set.Insert("list>li", SnippetData("ListBox > ListBoxItem", SnippetData::Filter_None, "<ListBox SelectedIndex=\"0\" Margin=\"5\">\n\t<ListBoxItem Content=\"${1:item1}\" />$2\n</ListBox>$4"));
    set.Insert("linearbrush", SnippetData("LinearGradientBrush", SnippetData::Filter_None, "<LinearGradientBrush StartPoint=\"0,0\" EndPoint=\"1,0\">\n\t<GradientStop Color=\"Gold\" Offset=\"0\" />\n\t<GradientStop Color=\"DarkOrange\" Offset=\"1\" />\n</LinearGradientBrush>$0"));
    set.Insert("radialbrush", SnippetData("RadialGradientBrush", SnippetData::Filter_None, "<RadialGradientBrush>\n\t<GradientStop Color=\"Gold\" Offset=\"0\"/>\n\t<GradientStop Color=\"DarkOrange\" Offset=\"1\"/>\n</RadialGradientBrush>$0"));
    set.Insert("imagebrush", SnippetData("ImageBrush", SnippetData::Filter_None, "<ImageBrush ImageSource=\"$1\" Stretch=\"${2:UniformToFill}\" />"));
    set.Insert("radio", SnippetData("RadioButton", SnippetData::Filter_None, "<RadioButton Width=\"120\" Height=\"50\" Content=\"${1:RadioButton}\" IsChecked=\"${2:True}\" Margin=\"5\" />$3"));
    set.Insert("label", SnippetData("Label", SnippetData::Filter_None, "<Label Content=\"${1:Label}\" FontSize=\"30\" Margin=\"5\" Foreground=\"OrangeRed\" />$0"));
    set.Insert("text", SnippetData("TextBlock", SnippetData::Filter_None, "<TextBlock Text=\"${1:TextBlock}\" FontSize=\"30\" Margin=\"5\" Foreground=\"OrangeRed\" />$0"));
    set.Insert("textbox", SnippetData("TextBox", SnippetData::Filter_None, "<TextBox Height=\"40\" Text=\"${1:TextBox}\" Margin=\"5\" />$0"));
    set.Insert("list", SnippetData("ListBox", SnippetData::Filter_None, "<ListBox ItemsSource=\"$1\" SelectedIndex=\"0\" Margin=\"5\" />$0"));
    set.Insert("content", SnippetData("ContentControl", SnippetData::Filter_None, "<ContentControl Content=\"$1\" Margin=\"5\" />$0"));
    set.Insert("combo", SnippetData("ComboBox", SnippetData::Filter_None, "<ComboBox Height=\"40\" ItemsSource=\"$1\" SelectedIndex=\"0\" Margin=\"5\" />$0"));
    set.Insert("combo>ci", SnippetData("ComboBox > ComboBoxItem", SnippetData::Filter_None, "<ComboBox Height=\"40\" SelectedIndex=\"0\" Margin=\"5\">\n\t<ComboBoxItem Content=\"${1:item1}\" />$2\n</ComboBox>$3"));
    set.Insert("border", SnippetData("Border", SnippetData::Filter_None, "<Border BorderBrush=\"${1:Black}\" BorderThickness=\"1\" Height=\"100\" Width=\"100\" />$0"));
    set.Insert("rect", SnippetData("Rectangle", SnippetData::Filter_None, "<Rectangle Width=\"300\" Height=\"200\" Margin=\"5\" Fill=\"Red\" />$0"));
    set.Insert("rect>fill", SnippetData("Rectangle > Fill", SnippetData::Filter_None, "<Rectangle Width=\"300\" Height=\"200\" Margin=\"5\" >\n\t<Rectangle.Fill>\n\t\t$1\n\t</Rectangle.Fill>\n</Rectangle>"));
    set.Insert("ellipse", SnippetData("Ellipse", SnippetData::Filter_None, "<Ellipse Width=\"300\" Height=\"300\" Margin=\"5\" Fill=\"Red\" />$0"));
    set.Insert("ellipse>fill", SnippetData("Ellipse > Fill", SnippetData::Filter_None, "<Ellipse Width=\"300\" Height=\"300\" Margin=\"5\">\n\t<Ellipse.Fill>\n\t\t$1\n\t</Ellipse.Fill>\n</Ellipse>$0"));
    set.Insert("path", SnippetData("Path", SnippetData::Filter_None, "<Path Stroke=\"Black\" Fill=\"Gray\" Data=\"M 10,100 C 10,300 300,-200 300,100\" />$0"));
    set.Insert("stack", SnippetData("StackPanel", SnippetData::Filter_None, "<StackPanel>\n\t$1\n</StackPanel>$0"));
    set.Insert("stack>button*3", SnippetData("StackPanel > Button*3", SnippetData::Filter_None, "<StackPanel>\n\t<Button Content=\"Button1\" Margin=\"5,5,5,0\" />\n\t<Button Content=\"Button2\" Margin=\"5,5,5,0\" />\n\t<Button Content=\"Button3\" Margin=\"5,5,5,0\" />$1\n</StackPanel>$0"));
    set.Insert("stackh", SnippetData("Horizontal StackPanel", SnippetData::Filter_None, "<StackPanel Orientation=\"Horizontal\">\n\t$1\n</StackPanel>$0"));
    set.Insert("stackh>button*3", SnippetData("Horizontal StackPanel > Button*3", SnippetData::Filter_None, "<StackPanel Orientation=\"Horizontal\">\n\t<Button Content=\"Button1\" Margin=\"5,5,5,0\" />\n\t<Button Content=\"Button2\" Margin=\"5,5,5,0\" />\n\t<Button Content=\"Button3\" Margin=\"5,5,5,0\" />$1\n</StackPanel>$0"));
    set.Insert("grid", SnippetData("Grid", SnippetData::Filter_None, "<Grid>\n\t$1\n</Grid>$0"));
    set.Insert("wrap", SnippetData("WrapPanel", SnippetData::Filter_None, "<WrapPanel>\n\t$1\n</WrapPanel>$0"));
    set.Insert("dock", SnippetData("DockPanel", SnippetData::Filter_None, "<DockPanel>\n\t$1\n</DockPanel>$0"));
    set.Insert("canvas", SnippetData("Canvas", SnippetData::Filter_None, "<Canvas>\n\t$1\n</Canvas>$0"));
    set.Insert("viewbox", SnippetData("Viewbox", SnippetData::Filter_None, "<Viewbox>\n\t$1\n</Viewbox>$0"));
    set.Insert("transformgrp", SnippetData("TransformGroup", SnippetData::Filter_None, "<TransformGroup>\n\t<ScaleTransform/>\n\t<SkewTransform/>\n\t<RotateTransform/>\n\t<TranslateTransform/>\n</TransformGroup>$0"));
    set.Insert("grid>rect*4", SnippetData("Grid > Rectangle*4", SnippetData::Filter_None, "<Grid Width=\"400\" Height=\"400\">\n\t<Grid.ColumnDefinitions>\n\t\t<ColumnDefinition Width=\"100\" />\n\t\t<ColumnDefinition Width=\"*\" />\n\t</Grid.ColumnDefinitions>\n\t<Grid.RowDefinitions>\n\t\t<RowDefinition Height=\"50\" />\n\t\t<RowDefinition Height=\"*\" />\n\t\t<RowDefinition Height=\"50\" />\n\t</Grid.RowDefinitions>\n\t<Rectangle Grid.Row=\"0\" Grid.Column=\"0\" Grid.ColumnSpan=\"2\" Fill=\"YellowGreen\" />\n\t<Rectangle Grid.Row=\"1\" Grid.Column=\"0\" Fill=\"Gray\" />\n\t<Rectangle Grid.Row=\"1\" Grid.Column=\"1\" Fill=\"Silver\" />\n\t<Rectangle Grid.Row=\"2\" Grid.Column=\"0\" Grid.ColumnSpan=\"2\" Fill=\"Orange\" />\n</Grid>$0"));
    set.Insert("controltmpl", SnippetData("Control Template", SnippetData::Filter_None, "<ControlTemplate TargetType=\"{x:Type ${1:Button}}\">\n\t$2\n</ControlTemplate>$0"));
    set.Insert("style", SnippetData("Style", SnippetData::Filter_None, "<Style TargetType=\"{x:Type ${1:Button}}\">\n\t<Setter Property=\"$2\" Value=\"$3\" />$4\n</Style>$0"));
    set.Insert("style>tmpl", SnippetData("Style with Template", SnippetData::Filter_None, "<Style TargetType=\"{x:Type ${1:Button}}\">\n\t<Setter Property=\"Template\">\n\t\t<Setter.Value>\n\t\t\t<ControlTemplate TargetType=\"{x:Type ${1:Button}}\">\n\t\t\t\t$2\n\t\t\t</ControlTemplate>\n\t\t</Setter.Value>\n\t</Setter>$3\n</Style>$0"));
    set.Insert("datatmpl", SnippetData("DataTemplate", SnippetData::Filter_None, "<DataTemplate DataType=\"{x:Type $1}\">\n\t$2\n</DataTemplate>$0"));

    set.Insert("controltmplkey", SnippetData("Control Template with Key", SnippetData::Filter_ResourceDictionary, "<ControlTemplate x:Key=\"$1\" TargetType=\"{x:Type ${2:Button}}\">\n\t$3\n</ControlTemplate>$0"));
    set.Insert("stylekey", SnippetData("Style with Key", SnippetData::Filter_ResourceDictionary, "<Style x:Key=\"$1\" TargetType=\"{x:Type ${2:Button}}\">\n\t<Setter Property=\"$3\" Value=\"$4\" />$5\n</Style>$0"));
    set.Insert("stylekey>tmpl", SnippetData("Style with Key > Template", SnippetData::Filter_ResourceDictionary, "<Style x:Key=\"$1\" TargetType=\"{x:Type ${2:Button}}\">\n\t<Setter Property=\"Template\">\n\t\t<Setter.Value>\n\t\t\t<ControlTemplate TargetType=\"{x:Type ${2:Button}}\">\n\t\t\t\t$3\n\t\t\t</ControlTemplate>\n\t\t</Setter.Value>\n\t</Setter>$4\n</Style>$0"));
    set.Insert("datatmplkey", SnippetData("DataTemplate with Key", SnippetData::Filter_ResourceDictionary, "<DataTemplate x:Key=\"$1\" DataType=\"{x:Type $2}\">\n\t$3\n</DataTemplate>$0"));

    set.Insert("root-userctrl", SnippetData("UserControl Root", SnippetData::Filter_Root, "<UserControl x:Class=\"$1\"\n\txmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\n\txmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\n\t$0\n</UserControl>"));
    set.Insert("root-grid", SnippetData("Grid Root", SnippetData::Filter_Root, "<Grid\n\txmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\n\txmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\n\t$1\n</Grid>$0"));
    set.Insert("root-page", SnippetData("Page Root", SnippetData::Filter_Root,
        "<Page\n\txmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\n\txmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\n\t$1\n</Page>$0"));

    set.Insert("sample-itemsctrl", SnippetData("ItemsControl Sample", SnippetData::Filter_Root, "<Grid\n\txmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"\n\txmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\">\n\t<Grid.Resources>\n\t\t<GradientStopCollection x:Key=\"list\">\n\t\t\t<GradientStop Offset=\"0\" Color=\"Red\"/>\n\t\t\t<GradientStop Offset=\"1\" Color=\"Green\"/>\n\t\t\t<GradientStop Offset=\"2\" Color=\"Blue\"/>\n\t\t</GradientStopCollection>\n\t\t<DataTemplate x:Key=\"itemTemplate\">\n\t\t\t<StackPanel Orientation=\"Horizontal\">\n\t\t\t\t<Rectangle Width=\"10\" Height=\"10\">\n\t\t\t\t\t<Rectangle.Fill>\n\t\t\t\t\t\t<SolidColorBrush Color=\"{Binding Color}\"/>\n\t\t\t\t\t</Rectangle.Fill>\n\t\t\t\t</Rectangle>\n\t\t\t\t<TextBlock Text=\"{Binding Offset}\" Margin=\"10,0,0,0\"/>\n\t\t\t</StackPanel>\n\t\t</DataTemplate>\n\t\t<ItemsPanelTemplate x:Key=\"itemsPanel\">\n\t\t\t<StackPanel/>\n\t\t</ItemsPanelTemplate>\n\t</Grid.Resources>\n\t<ItemsControl Width=\"100\" Height=\"100\"\n\t\tItemsSource=\"{StaticResource list}\"\n\t\tItemsPanel=\"{StaticResource itemsPanel}\"\n\t\tItemTemplate=\"{StaticResource itemTemplate}\"/>$0\n</Grid>"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void PopulateKeySnippets(SnippetSet& set)
{
    set.Insert("align", SnippetData("Horz. & Vert. Alignment", SnippetData::Filter_None, "HorizontalAlignment=\"${1:Center}\" VerticalAlignment=\"${2:Center}\"$0", "FrameworkElement"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void PopulateValueSnippets(SnippetSet& set)
{
    set.Insert("staticres", SnippetData("StaticResource", SnippetData::Filter_None, "{StaticResource ${1}}$0"));
    set.Insert("dynamicres", SnippetData("DynamicResource", SnippetData::Filter_None, "{DynamicResource ${1}}$0"));
    set.Insert("templatebind", SnippetData("TemplateBinding", SnippetData::Filter_ControlTemplate, "{TemplateBinding ${1:sourceProperty}}$0"));
    set.Insert("bind", SnippetData("Binding", SnippetData::Filter_None, "{Binding ${1:path}}$0"));
    set.Insert("bindconv", SnippetData("Converter Binding", SnippetData::Filter_None, "{Binding ${1:path}, Converter=$2}$0"));
    set.Insert("bindconvp", SnippetData("Converter Binding with Param", SnippetData::Filter_None, "{Binding ${1:path}, Converter=$2, ConverterParameter=$3}$0"));
    set.Insert("bindname", SnippetData("Element Name Binding", SnippetData::Filter_None, "{Binding ${1:path}, ElementName=$2}$0"));
    set.Insert("bindansc", SnippetData("Ancestor Binding", SnippetData::Filter_None, "{Binding ${1:path}, RelativeSource={RelativeSource AncestorType={x:Type ${2:Grid}}}}$0"));
    set.Insert("bindansclvl", SnippetData("Ancestor Binding", SnippetData::Filter_None, "{Binding ${1:path}, RelativeSource={RelativeSource AncestorType={x:Type ${2:Grid}}}, AncestorLevel=$3}}$0"));
    set.Insert("bindself", SnippetData("Self Binding", SnippetData::Filter_None, "{Binding ${1:path}, RelativeSource={RelativeSource Self}}$0"));
    set.Insert("bindtmpl", SnippetData("TemplateParent Binding", SnippetData::Filter_ControlTemplate, "{Binding ${1:path}, RelativeSource={RelativeSource TemplatedParent}}$0"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsValidCompletionType(const Noesis::Symbol typeName)
{
    if (Noesis::Factory::IsComponentRegistered(typeName))
    {
        return true;
    }
    const Noesis::Symbol converterTypeName = Noesis::Symbol(Noesis::FixedString<64>(
        Noesis::FixedString<64>::VarArgs(), "Converter<%s>", typeName.Str()).Str(),
        Noesis::Symbol::NullIfNotFound());
    return !converterTypeName.IsNull() && Noesis::Factory::IsComponentRegistered(converterTypeName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void AddCompletionTypeClass(void* user, const Noesis::Type* type)
{
    Noesis::BaseVector<const Noesis::TypeClass*>* types =
        static_cast<Noesis::BaseVector<const Noesis::TypeClass*>*>(user);

    const Noesis::TypeClass* typeClass = Noesis::DynamicCast<const Noesis::TypeClass*>(type);
    if (typeClass != nullptr && !typeClass->IsInterface()
        && IsValidCompletionType(type->GetTypeId()))
    {
        types->PushBack(typeClass);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static void AddTypeClass(void* user, const Noesis::Type* type)
{
    Noesis::BaseVector<const Noesis::TypeClass*>* types =
        static_cast<Noesis::BaseVector<const Noesis::TypeClass*>*>(user);

    const Noesis::TypeClass* typeClass = Noesis::DynamicCast<const Noesis::TypeClass*>(type);
    if (typeClass != nullptr)
    {
        types->PushBack(typeClass);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static const Noesis::TypeClass* GetCompletionTypeClass(const Noesis::Type* type)
{
    if (type == nullptr)
    {
        return nullptr;
    }
    const Noesis::TypeClass* typeClass = ExtractComponentType(type);
    if (typeClass == nullptr)
    {
        return nullptr;
    }
    return typeClass;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static const Noesis::TypeClass* GetCompletionTypeClass(const Noesis::Symbol typeId)
{
    return GetCompletionTypeClass(Noesis::Reflection::GetType(typeId));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static const Noesis::Type* GetEffectiveType(Noesis::Symbol typeName)
{
    if (Noesis::StrStartsWith(typeName.Str(), "System."))
    {
        if (typeName == Noesis::Symbol("System.Boolean"))
        {
            return Noesis::TypeOf<bool>();
        }
        if (typeName == Noesis::Symbol("System.Byte"))
        {
            return Noesis::TypeOf<uint8_t>();
        }
        if (typeName == Noesis::Symbol("System.SByte"))
        {
            return Noesis::TypeOf<int8_t>();
        }
        if (typeName == Noesis::Symbol("System.Int16"))
        {
            return Noesis::TypeOf<int16_t>();
        }
        if (typeName == Noesis::Symbol("System.Int32"))
        {
            return Noesis::TypeOf<int32_t>();
        }
        if (typeName == Noesis::Symbol("System.UInt16"))
        {
            return Noesis::TypeOf<uint16_t>();
        }
        if (typeName == Noesis::Symbol("System.UInt32"))
        {
            return Noesis::TypeOf<uint32_t>();
        }
        if (typeName == Noesis::Symbol("System.Single"))
        {
            return Noesis::TypeOf<float>();
        }
        if (typeName == Noesis::Symbol("System.Double"))
        {
            return Noesis::TypeOf<double>();
        }
        if (typeName == Noesis::Symbol("System.String"))
        {
            return Noesis::TypeOf<Noesis::String>();
        }
        if (typeName == Noesis::Symbol("System.TimeSpan"))
        {
            return Noesis::Reflection::GetType(Noesis::Symbol("TimeSpan"));
        }
    }
    return Noesis::Reflection::GetType(typeName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void EnumDictionaryKeys(const Noesis::ResourceDictionary* dict,
    const Noesis::Type* contentType, Noesis::Vector<Noesis::String>& resourceKeys,
    Noesis::HashSet<Noesis::String>& processedKeys)
{
    if (dict == nullptr)
    {
        return;
    }

    dict->EnumKeyValues([&contentType, &resourceKeys, &processedKeys](const char* key,
        Noesis::BaseComponent* value)
    {
        auto it = processedKeys.Find(key);
        if (it != processedKeys.End())
        {
            return;
        }

        processedKeys.Insert(key);

        Noesis::BoxedValue* boxed = Noesis::DynamicCast<Noesis::BoxedValue*>(value);
        const Noesis::Type* valueType;
        if (boxed != nullptr)
        {
            valueType = boxed->GetValueType();
        }
        else
        {
            valueType = value->GetClassType();
        }
        if (contentType->IsAssignableFrom(valueType))
        {
            resourceKeys.EmplaceBack(key);
        }
    });

    Noesis::ResourceDictionaryCollection* mergedDicts = dict->GetMergedDictionaries();
    for (int32_t i = 0; i < mergedDicts->Count(); i++)
    {
        EnumDictionaryKeys(mergedDicts->Get(i), contentType, resourceKeys, processedKeys);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void GetResources(const NoesisApp::XamlPart& part,
    const NoesisApp::LenientXamlParser::Parts& parts, const Noesis::Type* contentType,
    const Noesis::Uri& documentUri, bool dynamic, Noesis::Vector<Noesis::String>& resourceKeys)
{
    NS_DECLARE_SYMBOL(ResourceDictionary)
    NS_DECLARE_SYMBOL(Resources)
    NS_DECLARE_SYMBOL(Source)
    NS_DECLARE_SYMBOL(MergedDictionaries)

    Noesis::HashSet<Noesis::String> dictSources;
    Noesis::HashSet<Noesis::String> processedKeys;
    int32_t partIndex = part.partIndex != -1 ? part.partIndex : part.parentPartIndex;
    while (true)
    {
        if (parts[partIndex].HasFlag(NoesisApp::PartFlags_HasResources))
        {
            int32_t resNodePartIndex;

            if (parts[partIndex].partKind == NoesisApp::XamlPartKind_StartTagBegin
                && parts[partIndex].typeId == NSS(ResourceDictionary)
                && parts[partIndex].propertyId.IsNull())
            {
                resNodePartIndex = partIndex;
            }
            else
            {
                resNodePartIndex = partIndex + 1;
                for (; resNodePartIndex < (int32_t)parts.Size(); resNodePartIndex++)
                {
                    if (parts[resNodePartIndex].partKind == NoesisApp::XamlPartKind_StartTagBegin
                        && !parts[resNodePartIndex].HasErrorFlag(NoesisApp::ErrorFlags_Error)
                        && parts[resNodePartIndex].propertyId == NSS(Resources))
                    {
                        break;
                    }
                }
            }
            for (int32_t i = resNodePartIndex + 1; i < (int32_t)parts.Size(); i++)
            {
                const NoesisApp::XamlPart& checkPart = parts[i];
                if (checkPart.parentPartIndex < resNodePartIndex
                    || (!dynamic && i >= part.partIndex))
                {
                    break;
                }

                if (checkPart.partKind == NoesisApp::XamlPartKind_AttributeValue
                    && checkPart.HasFlag(NoesisApp::PartFlags_IsResource))
                {
                    // ToDo: Check owner type against resource type
                    const NoesisApp::XamlPart& owner =
                        parts[parts[checkPart.parentPartIndex].parentPartIndex];

                    auto it = processedKeys.Find(checkPart.content);
                    if (it == processedKeys.End())
                    {
                        processedKeys.Insert(checkPart.content);
                        if (contentType == Noesis::TypeOf<Noesis::BaseComponent>())
                        {
                            resourceKeys.EmplaceBack(checkPart.content);
                        }
                        else
                        {
                            const Noesis::Type* ownerType = GetEffectiveType(owner.typeId);

                            if (contentType->IsAssignableFrom(ownerType))
                            {
                                resourceKeys.EmplaceBack(checkPart.content);
                            }
                        }
                    }
                }
                else if (checkPart.partKind == NoesisApp::XamlPartKind_TagContent
                    || checkPart.partKind == NoesisApp::XamlPartKind_AttributeValue)
                {
                    const NoesisApp::XamlPart& keyPart = parts[checkPart.parentPartIndex];
                    if (!keyPart.HasErrorFlag(NoesisApp::ErrorFlags_Error)
                        && (keyPart.typeId == NSS(ResourceDictionary)
                            && keyPart.propertyId == NSS(Source)))
                    {
                        const NoesisApp::XamlPart& owner = parts[keyPart.parentPartIndex];
                        const NoesisApp::XamlPart& ancestor = parts[owner.parentPartIndex];

                        // ResourceDictionary, with Source
                        if (!owner.HasErrorFlag(NoesisApp::ErrorFlags_Error)
                            && !ancestor.HasErrorFlag(NoesisApp::ErrorFlags_Error)
                            && owner.typeId == NSS(ResourceDictionary)
                            && (ancestor.propertyId == NSS(Resources)
                                || ancestor.propertyId == NSS(MergedDictionaries)))
                        {
                            dictSources.Insert(checkPart.content);
                        }
                    }
                }
            }
        }

        if (partIndex == parts[partIndex].parentPartIndex)
        {
            break;
        }
        partIndex = parts[partIndex].parentPartIndex;
    }

    if (!dictSources.Empty())
    {
        Noesis::Ptr<Noesis::ResourceDictionary> dict = *new Noesis::ResourceDictionary();

        int32_t pos = Noesis::StrFindLast(documentUri.Str(), "/");
        NS_ASSERT(pos > -1);
        Noesis::String path(documentUri.Str(), documentUri.Str() + pos + 1);

        for (auto source : dictSources)
        {
            Noesis::Uri uri(source.Str());
            if (!uri.IsAbsolute())
            {
                uri = Noesis::Uri(path.Str(), source.Str());
            }

            dict->SetSource(uri);

            EnumDictionaryKeys(dict, contentType, resourceKeys, processedKeys);

            dict->Clear();
            dict->GetMergedDictionaries()->Clear();
        }
    }

    Noesis::ResourceDictionary* appResources = Noesis::GUI::GetApplicationResources();
    if (appResources != nullptr)
    {
        EnumDictionaryKeys(appResources, contentType, resourceKeys, processedKeys);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool TryFormatTypeName(const char* typeName,
    const NoesisApp::LenientXamlParser::PrefixMap& prefixMap, Noesis::BaseString& formattedName)
{
    if ((Noesis::StrStartsWith(typeName, "Base") && isupper(*(typeName + 4)))
        || Noesis::StrEndsWith(typeName, ">") || Noesis::StrEndsWith(typeName, "T")
        || Noesis::StrEndsWith(typeName, "MetaData"))
    {
        return false;
    }

    Noesis::FixedString<64> prefix;
    Noesis::FixedString<64> prefixNamespace;
    const int pos = Noesis::StrFindLast(typeName, ".");
    if (pos != -1)
    {
        prefixNamespace.Append(typeName, pos);
        const Noesis::Symbol clrNamespaceId = Noesis::Symbol(prefixNamespace.Str(),
            Noesis::Symbol::NullIfNotFound());
        if (!clrNamespaceId.IsNull())
        {
            for (auto& entry : prefixMap)
            {
                if (clrNamespaceId == entry.value.clrNamespaceId)
                {
                    prefix.Assign(entry.key.Str());
                }
            }
        }
        if (prefix.Empty())
        {
            if (Noesis::StrEquals(prefixNamespace.Str(), "NoesisGUIExtensions"))
            {
                prefix.Append("noesis");
            }
            else
            {
                prefix.PushBack(Noesis::ToLower(*prefixNamespace.Str()));
                prefix.Append(prefixNamespace.Begin() + 1);
            }
        }
        formattedName.Append(prefix);
        formattedName.PushBack(':');
        formattedName.Append(typeName + pos + 1);
    }
    else
    {
        formattedName.Append(typeName);
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void GeneratePropertyCompletionEntry(const Noesis::Symbol& nameSymbol,
    bool useFullyQualifiedName, const Noesis::TypeClass* ownerType,
    const NoesisApp::LenientXamlParser::PrefixMap& prefixMap,
    Noesis::HashSet<Noesis::String>& existingKeys, ItemSet& items)
{
    if (Noesis::StrStartsWith(nameSymbol.Str(), "."))
    {
        return;
    }

    Noesis::String formattedTypeName;
    if (!TryFormatTypeName(ownerType->GetName(), prefixMap, formattedTypeName))
    {
        return;
    }

    formattedTypeName.PushBack('.');
    formattedTypeName.Append(nameSymbol.Str());

    if (existingKeys.Find(formattedTypeName.Str()) != existingKeys.End())
    {
        return;
    }

    if (useFullyQualifiedName)
    {
        items.Insert(formattedTypeName.Str(), CompletionItemData(ItemKindProperty));
    }
    else
    {
        items.Insert(nameSymbol.Str(), CompletionItemData(ItemKindProperty));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void InsertColors(ItemSet& items)
{
    items.Insert("AliceBlue", CompletionItemData(ItemKindValue));
    items.Insert("AntiqueWhite", CompletionItemData(ItemKindValue));
    items.Insert("Aqua", CompletionItemData(ItemKindValue));
    items.Insert("Aquamarine", CompletionItemData(ItemKindValue));
    items.Insert("Azure", CompletionItemData(ItemKindValue));
    items.Insert("Beige", CompletionItemData(ItemKindValue));
    items.Insert("Bisque", CompletionItemData(ItemKindValue));
    items.Insert("Black", CompletionItemData(ItemKindValue));
    items.Insert("BlanchedAlmond", CompletionItemData(ItemKindValue));
    items.Insert("Blue", CompletionItemData(ItemKindValue));
    items.Insert("BlueViolet", CompletionItemData(ItemKindValue));
    items.Insert("Brown", CompletionItemData(ItemKindValue));
    items.Insert("BurlyWood", CompletionItemData(ItemKindValue));
    items.Insert("CadetBlue", CompletionItemData(ItemKindValue));
    items.Insert("Chartreuse", CompletionItemData(ItemKindValue));
    items.Insert("Chocolate", CompletionItemData(ItemKindValue));
    items.Insert("Coral", CompletionItemData(ItemKindValue));
    items.Insert("CornflowerBlue", CompletionItemData(ItemKindValue));
    items.Insert("Cornsilk", CompletionItemData(ItemKindValue));
    items.Insert("Crimson", CompletionItemData(ItemKindValue));
    items.Insert("Cyan", CompletionItemData(ItemKindValue));
    items.Insert("DarkBlue", CompletionItemData(ItemKindValue));
    items.Insert("DarkCyan", CompletionItemData(ItemKindValue));
    items.Insert("DarkGoldenrod", CompletionItemData(ItemKindValue));
    items.Insert("DarkGray", CompletionItemData(ItemKindValue));
    items.Insert("DarkGreen", CompletionItemData(ItemKindValue));
    items.Insert("DarkKhaki", CompletionItemData(ItemKindValue));
    items.Insert("DarkMagenta", CompletionItemData(ItemKindValue));
    items.Insert("DarkOliveGreen", CompletionItemData(ItemKindValue));
    items.Insert("DarkOrange", CompletionItemData(ItemKindValue));
    items.Insert("DarkOrchid", CompletionItemData(ItemKindValue));
    items.Insert("DarkRed", CompletionItemData(ItemKindValue));
    items.Insert("DarkSalmon", CompletionItemData(ItemKindValue));
    items.Insert("DarkSeaGreen", CompletionItemData(ItemKindValue));
    items.Insert("DarkSlateBlue", CompletionItemData(ItemKindValue));
    items.Insert("DarkSlateGray", CompletionItemData(ItemKindValue));
    items.Insert("DarkTurquoise", CompletionItemData(ItemKindValue));
    items.Insert("DarkViolet", CompletionItemData(ItemKindValue));
    items.Insert("DeepPink", CompletionItemData(ItemKindValue));
    items.Insert("DeepSkyBlue", CompletionItemData(ItemKindValue));
    items.Insert("DimGray", CompletionItemData(ItemKindValue));
    items.Insert("DodgerBlue", CompletionItemData(ItemKindValue));
    items.Insert("Firebrick", CompletionItemData(ItemKindValue));
    items.Insert("FloralWhite", CompletionItemData(ItemKindValue));
    items.Insert("ForestGreen", CompletionItemData(ItemKindValue));
    items.Insert("Fuchsia", CompletionItemData(ItemKindValue));
    items.Insert("Gainsboro", CompletionItemData(ItemKindValue));
    items.Insert("GhostWhite", CompletionItemData(ItemKindValue));
    items.Insert("Gold", CompletionItemData(ItemKindValue));
    items.Insert("Goldenrod", CompletionItemData(ItemKindValue));
    items.Insert("Gray", CompletionItemData(ItemKindValue));
    items.Insert("Green", CompletionItemData(ItemKindValue));
    items.Insert("GreenYellow", CompletionItemData(ItemKindValue));
    items.Insert("Honeydew", CompletionItemData(ItemKindValue));
    items.Insert("HotPink", CompletionItemData(ItemKindValue));
    items.Insert("IndianRed", CompletionItemData(ItemKindValue));
    items.Insert("Indigo", CompletionItemData(ItemKindValue));
    items.Insert("Ivory", CompletionItemData(ItemKindValue));
    items.Insert("Khaki", CompletionItemData(ItemKindValue));
    items.Insert("Lavender", CompletionItemData(ItemKindValue));
    items.Insert("LavenderBlush", CompletionItemData(ItemKindValue));
    items.Insert("LawnGreen", CompletionItemData(ItemKindValue));
    items.Insert("LemonChiffon", CompletionItemData(ItemKindValue));
    items.Insert("LightBlue", CompletionItemData(ItemKindValue));
    items.Insert("LightCoral", CompletionItemData(ItemKindValue));
    items.Insert("LightCyan", CompletionItemData(ItemKindValue));
    items.Insert("LightGoldenrodYellow", CompletionItemData(ItemKindValue));
    items.Insert("LightGray", CompletionItemData(ItemKindValue));
    items.Insert("LightGreen", CompletionItemData(ItemKindValue));
    items.Insert("LightPink", CompletionItemData(ItemKindValue));
    items.Insert("LightSalmon", CompletionItemData(ItemKindValue));
    items.Insert("LightSeaGreen", CompletionItemData(ItemKindValue));
    items.Insert("LightSkyBlue", CompletionItemData(ItemKindValue));
    items.Insert("LightSlateGray", CompletionItemData(ItemKindValue));
    items.Insert("LightSteelBlue", CompletionItemData(ItemKindValue));
    items.Insert("LightYellow", CompletionItemData(ItemKindValue));
    items.Insert("Lime", CompletionItemData(ItemKindValue));
    items.Insert("LimeGreen", CompletionItemData(ItemKindValue));
    items.Insert("Linen", CompletionItemData(ItemKindValue));
    items.Insert("Magenta", CompletionItemData(ItemKindValue));
    items.Insert("Maroon", CompletionItemData(ItemKindValue));
    items.Insert("MediumAquamarine", CompletionItemData(ItemKindValue));
    items.Insert("MediumBlue", CompletionItemData(ItemKindValue));
    items.Insert("MediumOrchid", CompletionItemData(ItemKindValue));
    items.Insert("MediumPurple", CompletionItemData(ItemKindValue));
    items.Insert("MediumSeaGreen", CompletionItemData(ItemKindValue));
    items.Insert("MediumSlateBlue", CompletionItemData(ItemKindValue));
    items.Insert("MediumSpringGreen", CompletionItemData(ItemKindValue));
    items.Insert("MediumTurquoise", CompletionItemData(ItemKindValue));
    items.Insert("MediumVioletRed", CompletionItemData(ItemKindValue));
    items.Insert("MidnightBlue", CompletionItemData(ItemKindValue));
    items.Insert("MintCream", CompletionItemData(ItemKindValue));
    items.Insert("MistyRose", CompletionItemData(ItemKindValue));
    items.Insert("Moccasin", CompletionItemData(ItemKindValue));
    items.Insert("NavajoWhite", CompletionItemData(ItemKindValue));
    items.Insert("Navy", CompletionItemData(ItemKindValue));
    items.Insert("OldLace", CompletionItemData(ItemKindValue));
    items.Insert("Olive", CompletionItemData(ItemKindValue));
    items.Insert("OliveDrab", CompletionItemData(ItemKindValue));
    items.Insert("Orange", CompletionItemData(ItemKindValue));
    items.Insert("OrangeRed", CompletionItemData(ItemKindValue));
    items.Insert("Orchid", CompletionItemData(ItemKindValue));
    items.Insert("PaleGoldenrod", CompletionItemData(ItemKindValue));
    items.Insert("PaleGreen", CompletionItemData(ItemKindValue));
    items.Insert("PaleTurquoise", CompletionItemData(ItemKindValue));
    items.Insert("PaleVioletRed", CompletionItemData(ItemKindValue));
    items.Insert("PapayaWhip", CompletionItemData(ItemKindValue));
    items.Insert("PeachPuff", CompletionItemData(ItemKindValue));
    items.Insert("Peru", CompletionItemData(ItemKindValue));
    items.Insert("Pink", CompletionItemData(ItemKindValue));
    items.Insert("Plum", CompletionItemData(ItemKindValue));
    items.Insert("PowderBlue", CompletionItemData(ItemKindValue));
    items.Insert("Purple", CompletionItemData(ItemKindValue));
    items.Insert("Red", CompletionItemData(ItemKindValue));
    items.Insert("RosyBrown", CompletionItemData(ItemKindValue));
    items.Insert("RoyalBlue", CompletionItemData(ItemKindValue));
    items.Insert("SaddleBrown", CompletionItemData(ItemKindValue));
    items.Insert("Salmon", CompletionItemData(ItemKindValue));
    items.Insert("SandyBrown", CompletionItemData(ItemKindValue));
    items.Insert("SeaGreen", CompletionItemData(ItemKindValue));
    items.Insert("SeaShell", CompletionItemData(ItemKindValue));
    items.Insert("Sienna", CompletionItemData(ItemKindValue));
    items.Insert("Silver", CompletionItemData(ItemKindValue));
    items.Insert("SkyBlue", CompletionItemData(ItemKindValue));
    items.Insert("SlateBlue", CompletionItemData(ItemKindValue));
    items.Insert("SlateGray", CompletionItemData(ItemKindValue));
    items.Insert("Snow", CompletionItemData(ItemKindValue));
    items.Insert("SpringGreen", CompletionItemData(ItemKindValue));
    items.Insert("SteelBlue", CompletionItemData(ItemKindValue));
    items.Insert("Tan", CompletionItemData(ItemKindValue));
    items.Insert("Teal", CompletionItemData(ItemKindValue));
    items.Insert("Thistle", CompletionItemData(ItemKindValue));
    items.Insert("Tomato", CompletionItemData(ItemKindValue));
    items.Insert("Transparent", CompletionItemData(ItemKindValue));
    items.Insert("Turquoise", CompletionItemData(ItemKindValue));
    items.Insert("Violet", CompletionItemData(ItemKindValue));
    items.Insert("Wheat", CompletionItemData(ItemKindValue));
    items.Insert("White", CompletionItemData(ItemKindValue));
    items.Insert("WhiteSmoke", CompletionItemData(ItemKindValue));
    items.Insert("Yellow", CompletionItemData(ItemKindValue));
    items.Insert("YellowGreen", CompletionItemData(ItemKindValue));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool TryGenerateTypeEnumEntries(const Noesis::Type* type,
    ItemSet& items)
{
    if (Noesis::TypeOf<Noesis::BaseNullable>()->IsAssignableFrom(type))
    {
        const Noesis::TypeNullable* typeNullable = static_cast<const Noesis::TypeNullable*>(type);
        type = typeNullable->type;
    }
    if (Noesis::TypeOf<bool>()->IsAssignableFrom(type))
    {
        items.Insert("True", CompletionItemData(ItemKindValue));
        items.Insert("False", CompletionItemData(ItemKindValue));
        return false;
    }
    if (Noesis::TypeOf<Noesis::Brush>()->IsAssignableFrom(type)
        || Noesis::TypeOf<Noesis::Color>()->IsAssignableFrom(type))
    {
        InsertColors(items);
        return false;
    }

    const Noesis::TypeEnum* typeEnum = Noesis::DynamicCast<const Noesis::TypeEnum*>(type);
    if (typeEnum == nullptr)
    {
        return false;
    }

    Noesis::ArrayRef<Noesis::TypeEnum::Value> typeNames = typeEnum->GetValues();
    for (auto entry : typeNames)
    {
        items.Insert(entry.first.Str(), CompletionItemData(ItemKindValue));
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool IsNullableType(const Noesis::Type* type)
{
    if (type == Noesis::TypeOf<Noesis::String>())
    {
        return true;
    }
    const Noesis::TypeClass* typeClass = Noesis::DynamicCast<const Noesis::TypeClass*>(type);
    return typeClass != nullptr
        && (typeClass->IsDescendantOf(Noesis::TypeOf<Noesis::BaseComponent>())
        || typeClass->IsDescendantOf(Noesis::TypeOf<Noesis::BaseNullable>()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void GenerateTypeCompletionEntries(const Noesis::Type* contentType,
    const NoesisApp::XamlPart& part, const NoesisApp::XamlPart& ownerPart, bool isContent,
    bool isDependencyProp, const NoesisApp::LenientXamlParser::PrefixMap& prefixMap, ItemSet& items)
{
    NS_DECLARE_SYMBOL(ResourceKey)
    NS_DECLARE_SYMBOL(ControlTemplate)

    const Noesis::TypeClass* contentTypeClass = Noesis::ExtractComponentType(contentType);
    if (contentTypeClass == nullptr && isContent)
    {
        return;
    }

    Noesis::Vector<const Noesis::Type*, 1> propertyContentTypes;

    if (contentTypeClass != nullptr)
    {
        if (Noesis::TypeOf<Noesis::BaseNullable>()->IsAssignableFrom(contentTypeClass))
        {
            const Noesis::TypeNullable* typeNullable =
                static_cast<const Noesis::TypeNullable*>(contentTypeClass);
            contentTypeClass =
                Noesis::DynamicCast<const Noesis::TypeClass*>(typeNullable->type);
        }

        const char* propertyContentTypeName = contentTypeClass->GetName();

        // ToDo: [maherne] Implement collection metadata
        if (Noesis::StrEndsWith(propertyContentTypeName, "Collection"))
        {
            if (Noesis::StrEquals(propertyContentTypeName, "ItemCollection"))
            {
                if (!part.HasFlag(NoesisApp::PartFlags_Extension))
                {
                    propertyContentTypes.PushBack(Noesis::TypeOf<Noesis::BaseComponent>());
                }
            }
            else
            {
                const Noesis::String collectionTypeName(propertyContentTypeName, 0,
                    static_cast<uint32_t>(strlen(propertyContentTypeName)) - 10);

                Noesis::Vector<const Noesis::TypeClass*, 1024> types;
                Noesis::Reflection::EnumTypes(&types, &AddTypeClass);
                for (const Noesis::TypeClass* typeClass : types)
                {
                    const char* typeName = typeClass->GetName();
                    const size_t typeLength = strlen(typeName);
                    if (Noesis::StrEndsWith(typeName, collectionTypeName.Str())
                        && (!part.HasFlag(NoesisApp::PartFlags_Extension)
                        || Noesis::TypeOf<Noesis::MarkupExtension>()->
                            IsAssignableFrom(typeClass))
                        && (typeLength == collectionTypeName.Size()
                            || *(typeName + typeLength - collectionTypeName.Size() - 1) == '.'))
                    {
                        propertyContentTypes.PushBack(typeClass);
                    }
                }
            }
        }
        else if (!part.HasFlag(NoesisApp::PartFlags_Extension)
            || Noesis::TypeOf<Noesis::MarkupExtension>()->IsAssignableFrom(contentTypeClass))
        {
            propertyContentTypes.PushBack(contentTypeClass);
        }
    }

    if (!isContent)
    {
        items.Insert(Noesis::TypeOf<Noesis::StaticResourceExtension>()->GetName(),
            CompletionItemData(ItemKindClass));

        if (IsNullableType(contentType))
        {
            items.Insert("x:Null", CompletionItemData(ItemKindClass));
        }

        if (part.HasFlag(NoesisApp::PartFlags_ControlTemplate)
            && ownerPart.typeId != NSS(ControlTemplate))
        {
            items.Insert("TemplateBinding", CompletionItemData(ItemKindClass));
        }

        const Noesis::TypeEnum* typeEnum =
            Noesis::DynamicCast<const Noesis::TypeEnum*>(contentType);
        if (typeEnum != nullptr)
        {
            items.Insert("x:Static", CompletionItemData(ItemKindClass));
        }

        if (contentType->GetTypeId() == Noesis::Symbol("const Type*"))
        {
            items.Insert("x:Type", CompletionItemData(ItemKindClass));
        }

        if (isDependencyProp)
        {
            items.Insert(Noesis::TypeOf<Noesis::DynamicResourceExtension>()->GetName(),
                CompletionItemData(ItemKindClass));
            items.Insert(Noesis::TypeOf<Noesis::Binding>()->GetName(),
                CompletionItemData(ItemKindClass));
        }
    }

    if (propertyContentTypes.Empty())
    {
        return;
    }

    Noesis::Vector<const Noesis::TypeClass*, 128> completionTypes;
    Noesis::Reflection::EnumTypes(&completionTypes, &AddCompletionTypeClass);
    for (const Noesis::TypeClass* typeClass : completionTypes)
    {
        for (const Noesis::Type* type : propertyContentTypes)
        {
            if (type->IsAssignableFrom(typeClass))
            {
                Noesis::String formattedTypeName;
                if (TryFormatTypeName(typeClass->GetName(), prefixMap, formattedTypeName))
                {
                    items.Insert(formattedTypeName.Str(), CompletionItemData(ItemKindClass));
                }
                break;
            }
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static bool HasUnclosedAncestor(NoesisApp::LenientXamlParser::Parts& parts,
    const NoesisApp::XamlPart& part, const NoesisApp::XamlPart& matchPart)
{
    if (part.partKind != NoesisApp::XamlPartKind_StartTagBegin)
    {
        return false;
    }
    if (!part.IsTypeMatch(matchPart))
    {
        return false;
    }
    if (part.HasErrorFlag(NoesisApp::ErrorFlags_NoEndTag))
    {
        return true;
    }
    if (part.partIndex != part.parentPartIndex)
    {
        return HasUnclosedAncestor(parts, parts[part.parentPartIndex], matchPart);
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void GetPartPropertyData(const Noesis::TypeClass* typeClass, NoesisApp::XamlPart& part,
    NoesisApp::TypePropertyMap& typePropertyMap,
    NoesisApp::DependencyPropertyMap& dependencyPropertyMap, bool isNewElement)
{
    while (typeClass != nullptr)
    {
        if (part.propertyId.IsNull() && isNewElement)
        {
            const Noesis::ContentPropertyMetaData* contentPropertyMetaData =
                Noesis::DynamicCast<Noesis::ContentPropertyMetaData*>(
                    typeClass->FindMeta(Noesis::TypeOf<Noesis::ContentPropertyMetaData>()));

            if (contentPropertyMetaData != nullptr)
            {
                part.propertyId = contentPropertyMetaData->GetContentProperty();
            }
        }

        NoesisApp::LangServerReflectionHelper::GetTypePropertyData(typeClass, typePropertyMap,
            dependencyPropertyMap);

        typeClass = typeClass->GetBase();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void GenerationCompletionMessage(int bodyId, ItemSet& validItems,
    const NoesisApp::XamlPart& part, const NoesisApp::LenientXamlParser::PrefixMap& prefixMap,
    bool returnAllTypes, bool returnSnippets, const Noesis::Type* filterType, bool inDictionary,
    bool addEqualsQuotations, bool isRoot, Noesis::BaseString& responseBuffer)
{
    if (returnAllTypes)
    {
        Noesis::Vector<const Noesis::TypeClass*, 128> completionTypes;
        Noesis::Reflection::EnumTypes(&completionTypes, &AddCompletionTypeClass);
        for (const Noesis::TypeClass* typeClass : completionTypes)
        {
            NS_ASSERT(typeClass != nullptr);
            if (filterType == nullptr || filterType->IsAssignableFrom(typeClass))
            {
                Noesis::String formattedTypeName;
                if (TryFormatTypeName(typeClass->GetName(), prefixMap, formattedTypeName))
                {
                    validItems.Insert(formattedTypeName.Str(), CompletionItemData(ItemKindClass));
                }
            }
        }
    }

    ItemSet items;

    const Noesis::Symbol controlTemplateId = Noesis::Symbol("ControlTemplate");

    for (const auto& itemEntry : validItems)
    {
        if (returnSnippets)
        {
            SnippetSet set;
            PopulateNodeSnippets(set);
            for (const auto& entry : set)
            {
                switch (entry.value.filter)
                {
                case SnippetData::Filter_Root:
                    {
                        if (!isRoot || part.partKind != NoesisApp::XamlPartKind_Undefined)
                        {
                            continue;
                        }
                        break;
                    }
                case SnippetData::Filter_ResourceDictionary:
                    {
                        if (!inDictionary)
                        {
                            continue;
                        }
                        break;
                    }
                case SnippetData::Filter_ControlTemplate:
                    {
                        if (part.typeId == controlTemplateId
                            || !part.HasFlag(NoesisApp::PartFlags_ControlTemplate))
                        {
                            continue;
                        }
                        break;
                    }
                default: break;
                }
                const char* termChar = entry.value.text.Begin() + 1 + itemEntry.key.Size();
                if ((*termChar == ' ' || *termChar == '>' || *termChar == '/' || *termChar == '\n')
                    && Noesis::StrStartsWith(entry.value.text.Begin() + 1, itemEntry.key.Str()))
                {
                    items.Insert(entry.key.Str(),
                        CompletionItemData(ItemKindSnippet, entry.value.detail.Str(),
                            entry.value));
                }
            }
        }
        else
        {
            if (!itemEntry.value.snippet.text.Empty())
            {
                switch (itemEntry.value.snippet.filter)
                {
                case SnippetData::Filter_Root:
                    {
                        if (!isRoot || part.partKind != NoesisApp::XamlPartKind_Undefined)
                        {
                            continue;
                        }
                        break;
                    }
                case SnippetData::Filter_ResourceDictionary:
                    {
                        if (!inDictionary)
                        {
                            continue;
                        }
                        break;
                    }
                case SnippetData::Filter_ControlTemplate:
                    {
                        if (part.typeId == controlTemplateId
                            || !part.HasFlag(NoesisApp::PartFlags_ControlTemplate))
                        {
                            continue;
                        }
                        break;
                    }
                default: break;
                }
            }
            items.Insert(itemEntry.key.Str(), itemEntry.value);
        }
    }

    NoesisApp::JsonBuilder result;

    result.StartObject();
    result.WritePropertyName("items");
    result.StartArray();

    for (const auto& entry : items)
    {
        result.StartObject();
        result.WritePropertyName("label");
        result.WriteValue(entry.key.Str());
        result.WritePropertyName("kind");
        result.WriteValue(entry.value.itemKind);

        if (!entry.value.snippet.text.Empty())
        {
            result.WritePropertyName("insertText");
            result.WriteValue(entry.value.snippet.text.Begin());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);
        }
        else if (addEqualsQuotations && entry.value.itemKind == ItemKindProperty)
        {
            const char* format;
            if (part.HasFlag(NoesisApp::PartFlags_Extension))
            {
                format = "%s=$0";
            }
            else
            {
                format = "%s=\"$1\"$0";
            }
            result.WritePropertyName("insertText");
            result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
                format, entry.key.Str()).Str());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);

            result.WritePropertyName("command");
            result.StartObject();
            result.WritePropertyName("title");
            result.WriteValue("Trigger Suggestions");
            result.WritePropertyName("command");
            result.WriteValue("noesisTool.tryTriggerSuggest");
            result.EndObject();
        }

        if (!entry.value.detail.Empty())
        {
            result.WritePropertyName("labelDetails");
            result.StartObject();
            result.WritePropertyName("detail");
            result.WriteValue(entry.value.detail.Str());
            result.EndObject();
        }

        result.EndObject();
    }

    if (part.partKind == NoesisApp::XamlPartKind_AttributeKey
        && !part.HasFlag(NoesisApp::PartFlags_Extension))
    {
        result.StartObject();
        result.WritePropertyName("label");
        result.WriteValue("x:Name");
        result.WritePropertyName("insertText");
        result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
            "x:Name=\"$0\"").Str());
        result.WritePropertyName("insertTextFormat");
        result.WriteValue(2);
        result.EndObject();

        if (inDictionary)
        {
            result.StartObject();
            result.WritePropertyName("label");
            result.WriteValue("x:Key");
            result.WritePropertyName("insertText");
            result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
                "x:Key=\"$0\"").Str());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);
            result.EndObject();
        }

        if (isRoot && part.partKind == NoesisApp::XamlPartKind_AttributeKey)
        {
            result.StartObject();
            result.WritePropertyName("label");
            result.WriteValue("x:Class");
            result.WritePropertyName("insertText");
            result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
                "x:Class=\"$0\"").Str());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);
            result.EndObject();
            
            result.StartObject();
            result.WritePropertyName("label");
            result.WriteValue("xmlns");
            result.WritePropertyName("insertText");
            result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
                "xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"").Str());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);
            result.EndObject();

            result.StartObject();
            result.WritePropertyName("label");
            result.WriteValue("xmlns:x");
            result.WritePropertyName("insertText");
            result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
                "xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\"").Str());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);
            result.EndObject();

            result.StartObject();
            result.WritePropertyName("label");
            result.WriteValue("xmlns:b");
            result.WritePropertyName("insertText");
            result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
                "xmlns:b=\"http://schemas.microsoft.com/xaml/behaviors\"").Str());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);
            result.EndObject();

            result.StartObject();
            result.WritePropertyName("label");
            result.WriteValue("xmlns:d");
            result.WritePropertyName("insertText");
            result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
                "xmlns:d=\"http://schemas.microsoft.com/expression/blend/2008\"\nxmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\"\nmc:Ignorable=\"d\"").Str());
            result.WritePropertyName("insertTextFormat");
            result.WriteValue(2);
            result.EndObject();
        }

        result.StartObject();
        result.WritePropertyName("label");
        result.WriteValue("xmlns:noesis");
        result.WritePropertyName("insertText");
        result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
            "xmlns:noesis=\"clr-namespace:NoesisGUIExtensions;assembly=Noesis.GUI.Extensions\"").Str());
        result.WritePropertyName("insertTextFormat");
        result.WriteValue(2);
        result.EndObject();

        result.StartObject();
        result.WritePropertyName("label");
        result.WriteValue("xmlns:system");
        result.WritePropertyName("insertText");
        result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
            "xmlns:system=\"clr-namespace:System;assembly=mscorlib\"").Str());
        result.WritePropertyName("insertTextFormat");
        result.WriteValue(2);
        result.EndObject();

        result.StartObject();
        result.WritePropertyName("label");
        result.WriteValue("xmlns:<custom>");
        result.WritePropertyName("insertText");
        result.WriteValue(Noesis::FixedString<64>(Noesis::FixedString<64>::VarArgs(),
            "xmlns:$1=\"clr-namespace:$2;assembly=$3\"$0").Str());
        result.WritePropertyName("insertTextFormat");
        result.WriteValue(2);
        result.EndObject();
    }

    result.EndArray();
    result.EndObject();

    NoesisApp::MessageWriter::CreateResponse(bodyId, result.Str(), responseBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void GetPosition(NoesisApp::DocumentContainer& document, uint32_t completionPosition,
    uint32_t& lineIndex, uint32_t& characterIndex)
{
    for (uint32_t line = 0; line < document.lineStartPositions.Size(); line++)
    {
        if (completionPosition >= document.lineStartPositions[line])
        {
            lineIndex = line;
            characterIndex = completionPosition - document.lineStartPositions[line];
        }
        else
        {
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::CapabilityCompletion::AutoInsertCloseRequest(int bodyId,
    DocumentContainer& document, const TextPosition& position, Noesis::BaseString& responseBuffer)
{
    if (position.character == 0)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid completion position.", responseBuffer);
        return;
    }

    const char* closeTrigger = document.text.Begin() + position.textPosition - 1;
    uint16_t state = 0;
    if (*closeTrigger == '>')
    {
        if (*(closeTrigger - 1) == '/')
        {
            MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                "RequestCancelled: Tag is already self-closed.", responseBuffer);
            return;
        }
        state = 1;
    }
    else if (*closeTrigger == '/')
    {
        if (*(closeTrigger - 1) == '<')
        {
            state = 2;
        }
        else
        {
            state = 3;
        }
    }

    LenientXamlParser::Parts parts;
    LenientXamlParser::LinePartIndicies linePartIndices;
    LenientXamlParser::NSDefinitionMap nsDefinitionMap;
    LenientXamlParser::ParseXaml(document, parts, linePartIndices, nsDefinitionMap);

    uint32_t lineIndex = 0;
    uint32_t characterIndex = 0;
    GetPosition(document, position.textPosition, lineIndex, characterIndex);

    XamlPart part;
    const FindXamlPartResult findPartResult = LenientXamlParser::FindPartAtPosition(parts,
        linePartIndices, lineIndex, characterIndex, true, part);

    if (findPartResult == FindXamlPartResult::None)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid completion position.", responseBuffer);
        return;
    }
    if (part.errorFlags > ErrorFlags_Error && !part.HasErrorFlag(ErrorFlags_MissingValue))
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid syntax", responseBuffer);
        return;
    }
    if (part.partKind == XamlPartKind_EndTag && !part.HasErrorFlag(ErrorFlags_IdError))
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: End tag already defined.", responseBuffer);
        return;
    }
    if (part.partKind != XamlPartKind_StartTagEnd && part.partKind != XamlPartKind_EndTag)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Closing context not found.", responseBuffer);
        return;
    }
    const XamlPart& parentPart = parts[part.parentPartIndex];
    if (parentPart.HasErrorFlag(ErrorFlags_IdError) && parentPart.content.Empty())
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid open tag.", responseBuffer);
        return;
    }
    if (state == 3)
    {
        if (!part.HasFlag(PartFlags_IsSelfEnding))
        {
            MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                "RequestCancelled: Failed to self-end.", responseBuffer);
            return;
        }
        if (!part.HasErrorFlag(ErrorFlags_MissingBracket))
        {
            MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                "RequestCancelled: Tag is already self-closed.", responseBuffer);
            return;
        }
    }
    else if (!HasUnclosedAncestor(parts, parentPart, parentPart))
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: No open tag found to close.", responseBuffer);
        return;
    }

    Noesis::FixedString<64> snippet;
    Noesis::FixedString<32> typeString;

    if (parentPart.HasErrorFlag(ErrorFlags_IdError))
    {
        typeString = parentPart.content.Str();
    }
    else
    {
        parentPart.GetTypeString(typeString);
    }
    switch (state)
    {
        case 1:
        {
            snippet.Format("</%s>", typeString.Str());
            break;
        }
        case 2:
        {
            snippet.Format("%s>", typeString.Str());
            break;
        }
        default:
        {
            snippet = ">";
            break;
        }
    }

    JsonBuilder result;
    result.StartObject();
    result.WritePropertyName("snippet");
    result.WriteValue(snippet.Str());
    result.EndObject();

    MessageWriter::CreateResponse(bodyId, result.Str(), responseBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::CapabilityCompletion::AutoInsertQuotesRequest(int bodyId,
    DocumentContainer& document, const TextPosition& position, Noesis::BaseString& responseBuffer)
{
    LenientXamlParser::Parts parts;
    LenientXamlParser::LinePartIndicies linePartIndices;
    LenientXamlParser::NSDefinitionMap nsDefinitionMap;
    LenientXamlParser::ParseXaml(document, parts, linePartIndices, nsDefinitionMap);
    XamlPart part;

    if (parts.Size() == 0)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Parsing failure.", responseBuffer);
        return;
    }

    uint32_t lineIndex = 0;
    uint32_t characterIndex = 0;
    GetPosition(document, position.textPosition, lineIndex, characterIndex);

    const FindXamlPartResult findPartResult = LenientXamlParser::FindPartAtPosition(parts,
        linePartIndices, lineIndex, characterIndex, true, part);

    if (findPartResult == FindXamlPartResult::None)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid completion position.", responseBuffer);
        return;
    }
    if (part.HasFlag(PartFlags_Extension))
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: In extension expression.", responseBuffer);
        return;
    }
    if (part.partKind != XamlPartKind_AttributeEquals)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid completion part.", responseBuffer);
        return;
    }
    if (parts[part.parentPartIndex].partKind != XamlPartKind_AttributeKey)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: No attribute key.", responseBuffer);
        return;
    }
    if (part.partIndex + 1 < static_cast<int32_t>(parts.Size())
        && parts[part.partIndex + 1].partKind == XamlPartKind_AttributeValue)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Quotes already exist.", responseBuffer);
        return;
    }
    if (part.partIndex + 1 < static_cast<int32_t>(parts.Size())
        && parts[part.partIndex + 1].partKind == XamlPartKind_AttributeEquals)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Equals already exist.", responseBuffer);
        return;
    }

    JsonBuilder result;
    result.StartObject();
    result.WritePropertyName("snippet");
    result.WriteValue("\"\"");
    result.EndObject();

    MessageWriter::CreateResponse(bodyId, result.Str(), responseBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void NoesisApp::CapabilityCompletion::CompletionRequest(int bodyId, DocumentContainer& document,
    const TextPosition& position, Noesis::BaseString& responseBuffer)
{
    NS_DECLARE_SYMBOL(ResourceKey)

    ItemSet items;

    LenientXamlParser::Parts parts;
    LenientXamlParser::LinePartIndicies linePartIndices;
    LenientXamlParser::NSDefinitionMap nsDefinitionMap;
    LenientXamlParser::ParseXaml(document, parts, linePartIndices, nsDefinitionMap);
    XamlPart part;

    LenientXamlParser::PrefixMap prefixMap;

    if (parts.Size() == 0)
    {
        GenerationCompletionMessage(bodyId, items, part, prefixMap, true, true,
            nullptr, false, false, true, responseBuffer);
        return;
    }

    uint32_t lineIndex = 0;
    uint32_t characterIndex = 0;
    GetPosition(document, position.textPosition, lineIndex, characterIndex);

    FindXamlPartResult findPartResult = LenientXamlParser::FindPartAtPosition(parts,
        linePartIndices, lineIndex, characterIndex, true, part);

    if (findPartResult == FindXamlPartResult::None)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid completion position.", responseBuffer);
        return;
    }

    bool isNewPart = false;
    bool returnSnippets = false;
    if (part.partKind == XamlPartKind_StartTagEnd)
    {
        if (part.HasFlag(PartFlags_Extension))
        {
            XamlPart& ancestorPart =
                parts[parts[parts[part.parentPartIndex].parentPartIndex].parentPartIndex];
            if (ancestorPart.HasFlag(PartFlags_Extension))
            {
                part.parentPartIndex = ancestorPart.partIndex;
                part.partKind = XamlPartKind_AttributeKey;
            }
            else
            {
                MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                    "RequestCancelled: Invalid completion position.", responseBuffer);
                return;
            }
        }
        else
        {
            returnSnippets = true;
            part.partKind = XamlPartKind_StartTagBegin;
            if (part.HasFlag(PartFlags_IsSelfEnding))
            {
                if (part.IsRoot())
                {
                    MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                        "RequestCancelled: Root node is self-closed.", responseBuffer);
                    return;
                }
                part.parentPartIndex = parts[part.parentPartIndex].parentPartIndex;
            }
        }
    }
    else if (part.partKind == XamlPartKind_EndTag)
    {
        if (parts[part.parentPartIndex].HasFlag(PartFlags_IsNodeProperty))
        {
            part.parentPartIndex = parts[part.parentPartIndex].parentPartIndex;
        }
        else
        {
            if (part.IsRoot())
            {
                MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                    "RequestCancelled: Root node is self-closed.", responseBuffer);
                return;
            }
            part.parentPartIndex = parts[part.parentPartIndex].parentPartIndex;
        }
        returnSnippets = true;
        part.partKind = XamlPartKind_StartTagBegin;
    }
    else if (part.partKind == XamlPartKind_TagContent)
    {
        returnSnippets = true;
        part.partKind = XamlPartKind_StartTagBegin;
    }
    else if (part.partKind == XamlPartKind_StartTagBegin)
    {
        if (lineIndex > part.endLineIndex
            || (lineIndex == part.endLineIndex && characterIndex > part.endCharacterIndex))
        {
            part.parentPartIndex = part.partIndex;
            isNewPart = true;

            if (part.HasFlag(PartFlags_Extension))
            {
                part.partKind = XamlPartKind_ExtensionParameter;
            }
            else
            {
                part.partKind = XamlPartKind_AttributeKey;
            }
        }
        else if (part.IsRoot())
        {
            GenerationCompletionMessage(bodyId, items, part, prefixMap, true, false,
                nullptr, false, false, true, responseBuffer);
            return;
        }
    }
    else if (part.partKind == XamlPartKind_AttributeKey
        || part.partKind == XamlPartKind_ExtensionParameter)
    {
        if (lineIndex > part.endLineIndex
            || (lineIndex == part.endLineIndex && characterIndex > part.endCharacterIndex))
        {
            if (part.partKind == XamlPartKind_ExtensionParameter)
            {
                part.partKind = XamlPartKind_AttributeKey;
            }
            else
            {
                MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                    "RequestCancelled: Invalid attribute position.", responseBuffer);
                return;
            }
        }
        if (parts[part.parentPartIndex].HasFlag(PartFlags_IsNodeProperty))
        {
            MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                "RequestCancelled: Node properties cannot have attributes.", responseBuffer);
            return;
        }
    }
    else if (part.partKind == XamlPartKind_AttributeEquals && part.HasFlag(PartFlags_Extension))
    {
        part.partKind = XamlPartKind_AttributeValue;
    }
    else if (part.partKind == XamlPartKind_AttributeValue)
    {
        if (lineIndex > part.endLineIndex
            || (lineIndex == part.endLineIndex && characterIndex > part.endCharacterIndex))
        {
            if (parts[part.parentPartIndex].partKind == XamlPartKind_AttributeKey)
            {
                part.parentPartIndex = parts[part.parentPartIndex].parentPartIndex;
            }
            else
            {
                part.parentPartIndex = parts[part.parentPartIndex].partIndex;
            }
            isNewPart = true;
            part.partKind = XamlPartKind_AttributeKey;
        }
        else if (parts[part.parentPartIndex].partKind != XamlPartKind_AttributeKey)
        {
            MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                "RequestCancelled: Invalid completion part.", responseBuffer);
            return;
        }
    }
    else
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid completion part.", responseBuffer);
        return;
    }

    if (!isNewPart && part.IsRoot())
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid root node.", responseBuffer);
        return;
    }

    LenientXamlParser::PopulatePrefixes(part.partIndex != -1
        ? part.partIndex
        : part.parentPartIndex, parts, nsDefinitionMap, prefixMap);

    XamlPart ownerPart = parts[part.parentPartIndex];
    if (ownerPart.HasErrorFlag(ErrorFlags_IdError))
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid owner id.", responseBuffer);
    }

    bool returnAllTypes = false;
    const Noesis::Type* filterType = nullptr;
    bool inDictionary = false;

    const Noesis::TypeClass* ownerTypeClass = Noesis::DynamicCast<const Noesis::TypeClass*>(
        Noesis::Reflection::GetType(ownerPart.typeId));
    if (ownerTypeClass == nullptr)
    {
        MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
            "RequestCancelled: Invalid owner type.", responseBuffer);
        return;
    }

    if (part.partKind == XamlPartKind_ExtensionParameter)
    {
        const Noesis::ContentPropertyMetaData* contentPropertyMetaData =
            Noesis::DynamicCast<Noesis::ContentPropertyMetaData*>(
                ownerTypeClass->FindMeta(Noesis::TypeOf<Noesis::ContentPropertyMetaData>()));

        if (contentPropertyMetaData != nullptr)
        {
            ownerPart.propertyId = contentPropertyMetaData->GetContentProperty();
        }
        else
        {
            part.partKind = XamlPartKind_AttributeKey;
        }
    }

    bool isContent = !returnSnippets && part.partKind == XamlPartKind_StartTagBegin
        && ownerPart.propertyId.IsNull();

    TypePropertyMap typeProperties;
    DependencyPropertyMap dependencyProperties;

    GetPartPropertyData(ownerTypeClass, ownerPart, typeProperties,
        dependencyProperties, part.partKind == XamlPartKind_StartTagBegin);

    const Noesis::TypeClass* resourceType = Noesis::TypeOf<Noesis::ResourceDictionary>();

    if (part.partKind == XamlPartKind_AttributeKey && !ownerPart.IsRoot())
    {
        XamlPart& ancestorPart = parts[parts[part.parentPartIndex].parentPartIndex];

        if (ancestorPart.HasErrorFlag(ErrorFlags_IdError))
        {
            MessageWriter::CreateErrorResponse(bodyId, LSPErrorCodes::RequestCancelled,
                "RequestCancelled: Invalid ancestor id.", responseBuffer);
        }

        const Noesis::TypeClass* parentTypeClass = GetCompletionTypeClass(ancestorPart.typeId);
        if (parentTypeClass != nullptr)
        {
            if (ancestorPart.propertyId.IsNull())
            {
                if (resourceType->IsAssignableFrom(parentTypeClass))
                {
                    inDictionary = true;
                }
            }
            else
            {
                TypePropertyMap parentTypeProperties;
                DependencyPropertyMap parentDependencyProperties;

                GetPartPropertyData(parentTypeClass, ancestorPart, parentTypeProperties,
                    parentDependencyProperties, false);

                auto tpIt = parentTypeProperties.Find(ancestorPart.propertyId);
                if (tpIt != parentTypeProperties.End())
                {
                    const Noesis::TypeClass* contentType =
                        GetCompletionTypeClass(tpIt->value->GetContentType());
                    if (contentType != nullptr
                        && resourceType->IsAssignableFrom(contentType))
                    {
                        inDictionary = true;
                    }
                }
                auto dpIt = dependencyProperties.Find(ownerPart.propertyId);
                if (dpIt != dependencyProperties.End())
                {
                    const Noesis::TypeClass* contentType =
                        GetCompletionTypeClass(dpIt->value->GetType());
                    if (contentType != nullptr
                        && resourceType->IsAssignableFrom(contentType))
                    {
                        inDictionary = true;
                    }
                }
            }
        }
    }

    bool addEqualsQuotations = false;
    if (isContent || part.partKind == XamlPartKind_AttributeKey
        || part.partKind == XamlPartKind_ExtensionParameter)
    {
        if (part.partKind == XamlPartKind_AttributeKey
            || part.partKind == XamlPartKind_ExtensionParameter)
        {
            SnippetSet set;
            PopulateKeySnippets(set);
            for (const auto& entry : set)
            {
                const Noesis::Type* snippetType = Noesis::Reflection::GetType(entry.value.typeId);
                if (snippetType == nullptr || snippetType->IsAssignableFrom(ownerTypeClass))
                {
                    items.Insert(entry.key.Str(), CompletionItemData(ItemKindSnippet,
                        entry.value.detail.Str(), entry.value));
                }
            }
            if (part.partIndex == -1
                || part.partIndex == static_cast<int32_t>(parts.Size()) - 1
                || parts[part.partIndex + 1].partKind != XamlPartKind_AttributeEquals)
            {
                addEqualsQuotations = true;
            }
        }

        Noesis::HashSet<Noesis::String> existingKeys;
        const int32_t length = static_cast<int32_t>(parts.Size());
        for (int32_t j = part.parentPartIndex + 1; j < length; j++)
        {
            XamlPart& checkPart = parts[j];
            if (checkPart.partKind == XamlPartKind_AttributeKey
                || part.partKind == XamlPartKind_ExtensionParameter)
            {
                if (checkPart.parentPartIndex == part.parentPartIndex)
                {
                    Noesis::String typeString;
                    checkPart.GetTypeString(typeString);
                    if (!typeString.Empty())
                    {
                        existingKeys.Insert(typeString);
                    }
                }
                else
                {
                    break;
                }
            }
            else if (checkPart.partKind == XamlPartKind_StartTagEnd
                || checkPart.partKind == XamlPartKind_EndTag)
            {
                break;
            }
        }
        for (auto& entry : typeProperties)
        {
            if (!entry.value->IsReadOnly()
                || Noesis::StrEndsWith(entry.value->GetContentType()->GetName(), "Collection")
                || Noesis::StrEndsWith(entry.value->GetContentType()->GetName(), "Collection*"))
            {
                GeneratePropertyCompletionEntry(entry.value->GetName(), isContent,
                    ownerTypeClass, prefixMap, existingKeys, items);
            }
        }
        for (auto& entry : dependencyProperties)
        {
            if (!entry.value->IsReadOnly()
                || Noesis::StrEndsWith(entry.value->GetType()->GetName(), "Collection"))
            {
                GeneratePropertyCompletionEntry(entry.value->GetName(), isContent,
                    ownerTypeClass, prefixMap, existingKeys, items);
            }
        }
    }

    if (part.partKind == XamlPartKind_AttributeValue
        || part.partKind == XamlPartKind_ExtensionParameter)
    {
        SnippetSet set;

        bool isDynamicResource =
            ownerTypeClass == Noesis::TypeOf<Noesis::DynamicResourceExtension>();
        if (ownerPart.propertyId == NSS(ResourceKey)
            && (isDynamicResource
                || ownerTypeClass == Noesis::TypeOf<Noesis::StaticResourceExtension>()))
        {
            XamlPart propPart;
            if (part.partKind == XamlPartKind_ExtensionParameter)
            {
                propPart = parts[ownerPart.parentPartIndex];
            }
            else
            {
                propPart = parts[parts[ownerPart.parentPartIndex].parentPartIndex];
            }
            const Noesis::TypeClass* propOwnerTypeClass = Noesis::DynamicCast<const
                Noesis::TypeClass*>(
                Noesis::Reflection::GetType(propPart.typeId));
            if (propOwnerTypeClass != nullptr)
            {
                TypePropertyMap propTypeProperties;
                DependencyPropertyMap propDependencyProperties;

                GetPartPropertyData(propOwnerTypeClass, propPart, propTypeProperties,
                    propDependencyProperties, false);

                const Noesis::Type* contentType = nullptr;
                auto dpIt = propDependencyProperties.Find(propPart.propertyId);
                if (dpIt != propDependencyProperties.End())
                {
                    contentType = dpIt->value->GetType();
                }
                else
                {
                    auto tpIt = propTypeProperties.Find(propPart.propertyId);
                    if (tpIt != propTypeProperties.End())
                    {
                        contentType = tpIt->value->GetContentType();
                    }
                }

                if (contentType != nullptr)
                {
                    const Noesis::TypeClass* typeClass = Noesis::ExtractComponentType(contentType);
                    if (typeClass != nullptr)
                    {
                        contentType = typeClass;
                    }

                    Noesis::Vector<Noesis::String> resourceKeys;
                    GetResources(part, parts, contentType, document.uri, isDynamicResource,
                        resourceKeys);

                    for (const Noesis::String& key : resourceKeys)
                    {
                        items.Insert(key.Str(), CompletionItemData(ItemKindValue));
                    }
                }
            }
        }
        else
        {
            PopulateValueSnippets(set);
        }

        auto tpIt = typeProperties.Find(ownerPart.propertyId);
        if (tpIt != typeProperties.End())
        {
            TryGenerateTypeEnumEntries(tpIt->value->GetContentType(), items);
            for (const auto& entry : set)
            {
                const Noesis::Type* snippetType = Noesis::Reflection::GetType(entry.value.typeId);
                if (snippetType == nullptr
                    || snippetType->IsAssignableFrom(tpIt->value->GetContentType()))
                {
                    items.Insert(entry.key.Str(), CompletionItemData(ItemKindSnippet,
                        entry.value.detail.Str(), entry.value));
                }
            }
        }
        auto dpIt = dependencyProperties.Find(ownerPart.propertyId);
        if (dpIt != dependencyProperties.End())
        {
            TryGenerateTypeEnumEntries(dpIt->value->GetType(), items);
            for (const auto& entry : set)
            {
                const Noesis::Type* snippetType = Noesis::Reflection::GetType(entry.value.typeId);
                if (snippetType == nullptr || snippetType->IsAssignableFrom(dpIt->value->GetType()))
                {
                    items.Insert(entry.key.Str(), CompletionItemData(ItemKindSnippet,
                        entry.value.detail.Str(), entry.value));
                }
            }
        }
    }

    if (part.partKind == XamlPartKind_StartTagBegin)
    {
        if (ownerPart.propertyId.IsNull())
        {
            if (resourceType->IsAssignableFrom(ownerTypeClass))
            {
                inDictionary = true;
                returnAllTypes = true;
            }
            else if (Noesis::TypeOf<Noesis::DataTemplate>()->IsAssignableFrom(ownerTypeClass)
                || Noesis::TypeOf<Noesis::ControlTemplate>()->IsAssignableFrom(ownerTypeClass))
            {
                returnAllTypes = true;
                filterType = Noesis::TypeOf<Noesis::UIElement>();
            }
        }
        else
        {
            auto dpIt = dependencyProperties.Find(ownerPart.propertyId);
            if (dpIt != dependencyProperties.End()
                && (returnSnippets || !dpIt->value->IsReadOnly() || isContent))
            {
                const Noesis::TypeClass* typeClass =
                    Noesis::ExtractComponentType(dpIt->value->GetType());
                if (resourceType->IsAssignableFrom(typeClass))
                {
                    inDictionary = true;
                    returnAllTypes = true;
                }
                else
                {
                    GenerateTypeCompletionEntries(dpIt->value->GetType(), part, ownerPart,
                        isContent, true, prefixMap, items);
                }
            }
            else
            {
                auto tpIt = typeProperties.Find(ownerPart.propertyId);
                if (tpIt != typeProperties.End()
                    && (returnSnippets || !tpIt->value->IsReadOnly() || isContent))
                {
                    const Noesis::TypeClass* typeClass = 
                        Noesis::ExtractComponentType(tpIt->value->GetContentType());
                    if (resourceType->IsAssignableFrom(typeClass))
                    {
                        inDictionary = true;
                        returnAllTypes = true;
                    }
                    else
                    {
                        GenerateTypeCompletionEntries(tpIt->value->GetContentType(), part,
                            ownerPart, isContent, false, prefixMap, items);
                    }
                }
            }
        }
    }

    GenerationCompletionMessage(bodyId, items, part, prefixMap, returnAllTypes,
        returnSnippets, filterType, inDictionary, addEqualsQuotations,
        ownerPart.IsRoot(), responseBuffer);
}

#endif
