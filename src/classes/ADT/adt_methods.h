#ifndef ADT_METHODS_H
#define ADT_METHODS_H

#include "vm.h"
#include "value.h"

// ADT Instance Methods (for ADT instances like Some(42))
value_t adt_instance_toString(vm_t* vm, int arg_count, value_t* args);
value_t adt_instance_equals(vm_t* vm, int arg_count, value_t* args);
value_t adt_instance_hash(vm_t* vm, int arg_count, value_t* args);

// ADT Class Static Methods (for constructor classes like Some, None)
value_t adt_class_toString(vm_t* vm, int arg_count, value_t* args);
value_t adt_class_equals(vm_t* vm, int arg_count, value_t* args);
value_t adt_class_hash(vm_t* vm, int arg_count, value_t* args);

#endif