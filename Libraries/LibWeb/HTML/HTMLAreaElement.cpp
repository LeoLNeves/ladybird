/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/ARIA/Roles.h>
#include <LibWeb/Bindings/HTMLAreaElement.h>
#include <LibWeb/DOM/DOMTokenList.h>
#include <LibWeb/HTML/HTMLAreaElement.h>
#include <LibWeb/HTML/Window.h>

namespace Web::HTML {

GC_DEFINE_ALLOCATOR(HTMLAreaElement);

HTMLAreaElement::HTMLAreaElement(DOM::Document& document, DOM::QualifiedName qualified_name)
    : HTMLElement(document, move(qualified_name))
{
}

HTMLAreaElement::~HTMLAreaElement() = default;

void HTMLAreaElement::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(HTMLAreaElement);
    Base::initialize(realm);
}

void HTMLAreaElement::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_rel_list);
}

void HTMLAreaElement::attribute_changed(FlyString const& name, Optional<String> const& old_value, Optional<String> const& value, Optional<FlyString> const& namespace_)
{
    Base::attribute_changed(name, old_value, value, namespace_);

    if (name == HTML::AttributeNames::href) {
        set_the_url();
    } else if (name == HTML::AttributeNames::rel) {
        if (m_rel_list)
            m_rel_list->associated_attribute_changed(value.value_or(String {}));
    }
}

// https://html.spec.whatwg.org/multipage/image-maps.html#dom-area-rellist
GC::Ref<DOM::DOMTokenList> HTMLAreaElement::rel_list()
{
    // The IDL attribute relList must reflect the rel content attribute.
    if (!m_rel_list)
        m_rel_list = DOM::DOMTokenList::create(*this, HTML::AttributeNames::rel);
    return *m_rel_list;
}

// https://html.spec.whatwg.org/multipage/interaction.html#dom-tabindex
i32 HTMLAreaElement::default_tab_index_value() const
{
    // See the base function for the spec comments.
    return 0;
}

Optional<ARIA::Role> HTMLAreaElement::default_role() const
{
    // https://www.w3.org/TR/html-aria/#el-area-no-href
    if (!href().is_empty())
        return ARIA::Role::link;
    // https://www.w3.org/TR/html-aria/#el-area
    return ARIA::Role::generic;
}

// https://html.spec.whatwg.org/multipage/image-maps.html#image-map-processing-model
bool HTMLAreaElement::is_point_inside(int x, int y) const
{
    auto shape_attr = get_attribute_value(HTML::AttributeNames::shape);
    auto coords_attr = get_attribute_value(HTML::AttributeNames::coords);

    // https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#lists-of-floating-point-numbers
    // Parse the coords attribute into a list of doubles
    Vector<double> coords;
    auto coord_strings = coords_attr.bytes_as_string_view().split_view_if([](char c) {
        // Delimiters are ASCII whitespace, U+002C COMMA, or U+003B SEMICOLON
        return c == ',' || c == ';' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
    });

    for (auto const& part : coord_strings) {
        if (auto value = part.to_number<double>(); value.has_value()) {
            coords.append(*value);
        } else {
            // If number is an error, set zero instead.
            coords.append(0.0);
        }
    }

    // https://html.spec.whatwg.org/multipage/image-maps.html#attr-area-shape-default
    // Default state: covers the whole image
    if (shape_attr.equals_ignoring_ascii_case("default"sv)) {
        return true;
    }

    // https://html.spec.whatwg.org/multipage/image-maps.html#attr-area-shape-circle
    // Circle state
    if (shape_attr.equals_ignoring_ascii_case("circle"sv) || shape_attr.equals_ignoring_ascii_case("circ"sv)) {
        if (coords.size() < 3)
            return false;
        int cx = coords[0];
        int cy = coords[1];
        int radius = coords[2];
        if (radius <= 0)
            return false;

        double dx = x - cx;
        double dy = y - cy;
        return (dx * dx + dy * dy) <= ((double)radius * radius);
    }

    // https://html.spec.whatwg.org/multipage/image-maps.html#attr-area-shape-poly
    // Polygon state
    if (shape_attr.equals_ignoring_ascii_case("poly"sv) || shape_attr.equals_ignoring_ascii_case("polygon"sv)) {
        if (coords.size() < 6)
            return false;
        bool inside = false;
        int n = coords.size() / 2;

        for (int i = 0, j = n - 1; i < n; j = i++) {
            int xi = coords[i * 2];
            int yi = coords[i * 2 + 1];
            int xj = coords[j * 2];
            int yj = coords[j * 2 + 1];

            if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
                inside = !inside;
            }
        }
        return inside;
    }

    // https://html.spec.whatwg.org/multipage/image-maps.html#attr-area-shape-rect
    // Rectangle state (all other cases default to rect)
    if (coords.size() >= 4) {
        int x1 = coords[0];
        int y1 = coords[1];
        int x2 = coords[2];
        int y2 = coords[3];

        if (x1 > x2)
            AK::swap(x1, x2);
        if (y1 > y2)
            AK::swap(y1, y2);

        return x >= x1 && x <= x2 && y >= y1 && y <= y2;
    }

    return false;
}

}
