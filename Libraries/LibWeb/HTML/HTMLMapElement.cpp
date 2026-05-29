/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/HTMLMapElement.h>
#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/HTML/HTMLAreaElement.h>
#include <LibWeb/HTML/HTMLMapElement.h>

namespace Web::HTML {

GC_DEFINE_ALLOCATOR(HTMLMapElement);

HTMLMapElement::HTMLMapElement(DOM::Document& document, DOM::QualifiedName qualified_name)
    : HTMLElement(document, move(qualified_name))
{
}

HTMLMapElement::~HTMLMapElement() = default;

void HTMLMapElement::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(HTMLMapElement);
    Base::initialize(realm);
}

void HTMLMapElement::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_areas);
}

// https://html.spec.whatwg.org/multipage/image-maps.html#dom-map-areas
GC::Ref<DOM::HTMLCollection> HTMLMapElement::areas()
{
    // The areas attribute must return an HTMLCollection rooted at the map element, whose filter matches only area elements.
    if (!m_areas) {
        m_areas = DOM::HTMLCollection::create(*this, DOM::HTMLCollection::Scope::Descendants, [](Element const& element) {
            return is<HTML::HTMLAreaElement>(element);
        });
    }
    return *m_areas;
}

HTMLAreaElement* HTMLMapElement::area_at(int x, int y) const
{
    HTMLAreaElement* result = nullptr;

    const_cast<HTMLMapElement*>(this)->template for_each_in_subtree_of_type<HTML::HTMLAreaElement>([&](auto& area) {
        if (area.is_point_inside(x, y)) {
            result = &area;
            return TraversalDecision::Break;
        }
        return TraversalDecision::Continue;
    });

    return result;
}

}
