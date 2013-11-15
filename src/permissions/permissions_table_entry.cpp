#include <permissions/permissions.hpp>


using namespace MCPP::PermissionsImpl;


namespace MCPP {


	PermissionsTableEntry::PermissionsTableEntry () noexcept : Full(false) {	}


	PermissionsTableEntry::PermissionsTableEntry (String name, const PermissionsSet & set)
		:	Name(std::move(name)),
			Full(set.Individual.Full()),
			Difference(set.Individual.Count()),
			Groups(set.Groups.size())
	{
	
		for (auto & s : set.Individual) Difference.Add(s);
		
		for (auto & s : set.Groups) Groups.Add(s);
	
	}


}
