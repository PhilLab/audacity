/**********************************************************************

  Audacity: A Digital Audio Editor

  XMLMethodRegistry.cpp

  Paul Licameli

**********************************************************************/

#include "XMLMethodRegistry.h"

#include "Identifier.h"
#include <wx/string.h>

XMLMethodRegistryBase::XMLMethodRegistryBase() = default;
XMLMethodRegistryBase::~XMLMethodRegistryBase() = default;

void XMLMethodRegistryBase::Register(
   std::string tag, TypeErasedObjectAccessor accessor )
{
   // Store string in a separate container from the map, so the map
   // can be keyed by string_view.
   // Beware small-string optimization!  Be sure strings don't relocate for
   // growth of the container.  Use a list, not a vector.
   auto &newtag = mTags.emplace_front(move(tag));
   mTagTable[ newtag ] = move( accessor );
}

XMLTagHandler *XMLMethodRegistryBase::CallObjectAccessor(
   const std::string_view &tag, void *p )
{
   const auto &table = mTagTable;
   if (auto iter = table.find( tag ); iter != table.end())
      if (auto &fn = iter->second)
         return fn( p );
   return nullptr;
}

void XMLMethodRegistryBase::PushAccessor( TypeErasedAccessor accessor )
{
   mAccessors.emplace_back( move( accessor ) );
}

void XMLMethodRegistryBase::Register(
   std::string tag, TypeErasedMutator mutator )
{
   // Similar to the other overload of Register
   auto &newtag = mMutatorTags.emplace_front(move(tag));
   mMutatorTable[ newtag ] = { mAccessors.size() - 1, move( mutator ) };
}

bool XMLMethodRegistryBase::CallAttributeHandler( const std::string_view &tag,
      void *p, const XMLAttributeValueView &value )
{
   const auto &table = mMutatorTable;
   if (auto iter = table.find(tag); iter != table.end())
      // Tag is known
      if (auto &pair = iter->second;
          pair.second && pair.first < mAccessors.size() )
         // Mutator is not null and accessor exists
         if (auto &accessor = mAccessors[pair.first])
            // Accessor is not null; compose accessor and mutator
            return pair.second( accessor( p ), value ), true;
   return false;
}

void XMLMethodRegistryBase::Register( TypeErasedWriter writer )
{
   mWriterTable.emplace_back( move( writer ) );
}

void XMLMethodRegistryBase::CallWriters( const void *p, XMLWriter &writer )
{
   const auto &table = mWriterTable;
   for ( auto &fn : table )
      if (fn)
         fn( p, writer );
}
