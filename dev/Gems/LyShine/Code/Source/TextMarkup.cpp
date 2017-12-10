/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "StdAfx.h"
#include "TextMarkup.h"
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>

namespace
{
    //! Takes an input source string and wraps it for XML parsing
    void InsertMarkup(const AZStd::string& sourceBuffer, AZStd::string& targetBuffer)
    {
        targetBuffer = "<root>" + sourceBuffer + "</root>";

        AZStd::string::size_type pos = targetBuffer.find(">");
        while (pos != AZStd::string::npos)
        {
            ++pos;
            if (pos < targetBuffer.length())
            {
                AZStd::string::value_type nextChar = targetBuffer.at(pos);

                if (nextChar != '<')
                {
                    static const AZStd::string CharStartTag("<ch value=\"");
                    targetBuffer.insert(pos, CharStartTag);
                    pos += CharStartTag.length();

                    AZStd::string::size_type substrStart = pos;

                    pos = targetBuffer.find("<", pos);

                    if (AZStd::string::npos != pos)
                    {
                        AZStd::string::size_type substrEnd = pos;

                        static const AZStd::string CharEndTag("\" />");
                        targetBuffer.insert(pos, CharEndTag);
                        pos += CharEndTag.length();
                    }
                }
            }

            pos = targetBuffer.find(">", pos);
        }

        // Newlines need to be escaped or the XML parser could toss them out
        targetBuffer = AZStd::regex_replace(targetBuffer, AZStd::regex("\n"), "\\n");
    }

    //! Takes an TextMarkup::Tag and dumps all character data to the given output string buffer.
    void DumpCharData(const TextMarkup::Tag& markupRootTag, AZStd::string& ouputText)
    {
        AZStd::stack<const TextMarkup::Tag*> tagStack;
        tagStack.push(&markupRootTag);

        while (!tagStack.empty())
        {
            const TextMarkup::Tag* curTag = tagStack.top();
            tagStack.pop();

            auto riter = curTag->children.rbegin();
            for (; riter != curTag->children.rend(); ++riter)
            {
                tagStack.push(*riter);
            }

            TextMarkup::TagType type = curTag->GetType();

            if (type == TextMarkup::TagType::Text)
            {
                const TextMarkup::TextTag* pText = static_cast<const TextMarkup::TextTag*>(curTag);
                ouputText += pText->text;
            }
        }
    }

    //! Serializes a given XML root object to an TextMarkup tag tree.
    bool PopulateTagTreeFromXml(const XmlNodeRef& node, TextMarkup::Tag& markupTag)
    {
        if (!node)
        {
            return false;
        }

        TextMarkup::Tag* newTag = &markupTag;

        if (AZStd::string(node->getTag()) == "b")
        {
            newTag = new TextMarkup::BoldTag();
        }
        else if (AZStd::string(node->getTag()) == "i")
        {
            newTag = new TextMarkup::ItalicTag();
        }
        else if (AZStd::string(node->getTag()) == "font")
        {
            const int numAttributes = node->getNumAttributes();

            if (numAttributes <= 0)
            {
                // Expecting at least one attribute
                return false;
            }

            AZStd::string face;
            AZ::Vector3 color(TextMarkup::ColorInvalid);

            for (int i = 0, count = node->getNumAttributes(); i < count; ++i)
            {
                const char* key = "";
                const char* value = "";
                if (node->getAttributeByIndex(i, &key, &value))
                {
                    if (AZStd::string(key) == "face")
                    {
                        face = value;
                    }
                    else if (AZStd::string(key) == "color")
                    {
                        AZStd::string colorValue(string(value).Trim());
                        AZStd::string::size_type ExpectedNumChars = 7;
                        if (ExpectedNumChars == colorValue.size() && '#' == colorValue.at(0))
                        {
                            static const int base16 = 16;
                            static const float normalizeRgbMultiplier = 1.0f / 255.0f;
                            color.SetX(static_cast<float>(AZStd::stoi(colorValue.substr(1, 2), 0, base16)) * normalizeRgbMultiplier);
                            color.SetY(static_cast<float>(AZStd::stoi(colorValue.substr(3, 2), 0, base16)) * normalizeRgbMultiplier);
                            color.SetZ(static_cast<float>(AZStd::stoi(colorValue.substr(5, 2), 0, base16)) * normalizeRgbMultiplier);
                        }
                    }
                    else
                    {
                        // Unexpected font tag attribute
                        return false;
                    }
                }
            }

            TextMarkup::FontTag* fontTag = new TextMarkup::FontTag();
            newTag = fontTag;

            fontTag->face = face;
            fontTag->color = color;
        }
        else if (AZStd::string(node->getTag()) == "ch")
        {
            AZStd::string nodeContent;

            const char* key = "";
            const char* value = "";
            if (!node->getAttributeByIndex(0, &key, &value))
            {
                return false;
            }

            if (AZStd::string(key) == "value")
            {
                nodeContent = value;
            }
            else
            {
                // Unexpected attribute
                return false;
            }

            TextMarkup::TextTag* textTag = new TextMarkup::TextTag();
            newTag = textTag;

            textTag->text = nodeContent;

        }
        else if (AZStd::string(node->getTag()) == "root")
        {
            // Ignore
        }
        else
        {
            return false;
        }

        // Guard against a tag adding itself as its own child
        if (newTag != &markupTag)
        {
            markupTag.children.push_back(newTag);
        }

        for (int i = 0, count = node->getChildCount(); i < count; ++i)
        {
            XmlNodeRef child = node->getChild(i);
            if (!PopulateTagTreeFromXml(child, *newTag))
            {
                return false;
            }
        }

        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool TextMarkup::ParseMarkupBuffer(const AZStd::string& sourceBuffer, TextMarkup::Tag& markupTag, bool suppressWarnings)
{
    // First, wrap up the source text to make it parseable XML
    AZStd::string wrappedSourceText(sourceBuffer);
    InsertMarkup(sourceBuffer, wrappedSourceText);

    // Parse the wrapped text as XML
    XmlNodeRef xmlRoot = GetISystem()->LoadXmlFromBuffer(wrappedSourceText.c_str(), wrappedSourceText.length(), false, suppressWarnings);

    if (xmlRoot)
    {
        return PopulateTagTreeFromXml(xmlRoot, markupTag);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void TextMarkup::CopyCharData(const AZStd::string& sourceBuffer, AZStd::string& targetBuffer)
{
    TextMarkup::Tag markupRootTag;
    if (ParseMarkupBuffer(sourceBuffer, markupRootTag))
    {
        DumpCharData(markupRootTag, targetBuffer);
    }
    
    if (targetBuffer.empty())
    {
        // If, for some reason we couldn't parse the text as XML, we simply
        // assign the source buffer
        targetBuffer = sourceBuffer;
    }
}

#include "Tests/internal/test_TextMarkup.cpp"
