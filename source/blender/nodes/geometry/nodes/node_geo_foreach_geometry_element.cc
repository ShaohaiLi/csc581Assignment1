/* SPDX-FileCopyrightText: 2024 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "node_geometry_util.hh"

#include "BLI_string_utf8.h"

#include "BLO_read_write.hh"

#include "RNA_access.hh"
#include "RNA_prototypes.hh"

#include "NOD_geo_foreach_geometry_element.hh"
#include "NOD_node_extra_info.hh"
#include "NOD_socket_items_ops.hh"

#include "UI_interface.hh"
#include "UI_resources.hh"

#include "BKE_screen.hh"

#include "WM_api.hh"

namespace blender::nodes::node_geo_foreach_geometry_element_cc {

static void draw_item(uiList * /*ui_list*/,
                      const bContext *C,
                      uiLayout *layout,
                      PointerRNA * /*idataptr*/,
                      PointerRNA *itemptr,
                      int /*icon*/,
                      PointerRNA * /*active_dataptr*/,
                      const char * /*active_propname*/,
                      int /*index*/,
                      int /*flt_flag*/)
{
  uiLayout *row = uiLayoutRow(layout, true);
  float4 color;
  RNA_float_get_array(itemptr, "color", color);
  uiTemplateNodeSocket(row, const_cast<bContext *>(C), color);
  uiLayoutSetEmboss(row, UI_EMBOSS_NONE);
  uiItemR(row, itemptr, "name", UI_ITEM_NONE, "", ICON_NONE);
}

/** Shared between zone input and output node. */
static void node_layout_ex(uiLayout *layout, bContext *C, PointerRNA *current_node_ptr)
{
  bNodeTree &ntree = *reinterpret_cast<bNodeTree *>(current_node_ptr->owner_id);
  bNode *current_node = static_cast<bNode *>(current_node_ptr->data);

  const bke::bNodeTreeZones *zones = ntree.zones();
  if (!zones) {
    return;
  }
  const bke::bNodeTreeZone *zone = zones->get_zone_by_node(current_node->identifier);
  if (!zone) {
    return;
  }
  if (!zone->output_node) {
    return;
  }
  const bool is_zone_input_node = current_node->type == GEO_NODE_FOREACH_GEOMETRY_ELEMENT_INPUT;
  bNode &output_node = const_cast<bNode &>(*zone->output_node);
  PointerRNA output_node_ptr = RNA_pointer_create(
      current_node_ptr->owner_id, &RNA_Node, &output_node);
  auto &storage = *static_cast<NodeGeometryForeachGeometryElementOutput *>(output_node.storage);

  if (is_zone_input_node) {
    if (uiLayout *panel = uiLayoutPanel(C, layout, "input", false, TIP_("Input Fields"))) {
      static const uiListType *input_items_list = []() {
        uiListType *list = MEM_cnew<uiListType>(__func__);
        STRNCPY(list->idname, "DATA_UL_foreach_geometry_element_input_items");
        list->draw_item = draw_item;
        WM_uilisttype_add(list);
        return list;
      }();
      uiLayout *row = uiLayoutRow(panel, false);
      uiTemplateList(row,
                     C,
                     input_items_list->idname,
                     "",
                     &output_node_ptr,
                     "input_items",
                     &output_node_ptr,
                     "active_input_index",
                     nullptr,
                     3,
                     5,
                     UILST_LAYOUT_DEFAULT,
                     0,
                     UI_TEMPLATE_LIST_FLAG_NONE);
      {
        uiLayout *ops_col = uiLayoutColumn(row, false);
        {
          uiLayout *add_remove_col = uiLayoutColumn(ops_col, true);
          uiItemO(
              add_remove_col, "", ICON_ADD, "node.foreach_geometry_element_zone_input_item_add");
          uiItemO(add_remove_col,
                  "",
                  ICON_REMOVE,
                  "node.foreach_geometry_element_zone_input_item_remove");
        }
        {
          uiLayout *up_down_col = uiLayoutColumn(ops_col, true);
          uiItemEnumO(up_down_col,
                      "node.foreach_geometry_element_zone_input_item_move",
                      "",
                      ICON_TRIA_UP,
                      "direction",
                      0);
          uiItemEnumO(up_down_col,
                      "node.foreach_geometry_element_zone_input_item_move",
                      "",
                      ICON_TRIA_DOWN,
                      "direction",
                      1);
        }
      }

      if (storage.input_items.active_index >= 0 &&
          storage.input_items.active_index < storage.input_items.items_num)
      {
        NodeForeachGeometryElementInputItem &active_item =
            storage.input_items.items[storage.input_items.active_index];
        PointerRNA item_ptr = RNA_pointer_create(
            output_node_ptr.owner_id,
            ForeachGeometryElementInputItemsAccessor::item_srna,
            &active_item);
        uiLayoutSetPropSep(panel, true);
        uiLayoutSetPropDecorate(panel, false);
        uiItemR(panel, &item_ptr, "socket_type", UI_ITEM_NONE, nullptr, ICON_NONE);
      }
    }
  }
  else {
    if (uiLayout *panel = uiLayoutPanel(C, layout, "main_items", false, TIP_("Main Geometry"))) {
      static const uiListType *main_items_list = []() {
        uiListType *list = MEM_cnew<uiListType>(__func__);
        STRNCPY(list->idname, "DATA_UL_foreach_geometry_element_main_items");
        list->draw_item = draw_item;
        WM_uilisttype_add(list);
        return list;
      }();
      uiLayout *row = uiLayoutRow(panel, false);
      uiTemplateList(row,
                     C,
                     main_items_list->idname,
                     "",
                     &output_node_ptr,
                     "main_items",
                     &output_node_ptr,
                     "active_main_index",
                     nullptr,
                     3,
                     5,
                     UILST_LAYOUT_DEFAULT,
                     0,
                     UI_TEMPLATE_LIST_FLAG_NONE);
      {
        uiLayout *ops_col = uiLayoutColumn(row, false);
        {
          uiLayout *add_remove_col = uiLayoutColumn(ops_col, true);
          uiItemO(
              add_remove_col, "", ICON_ADD, "node.foreach_geometry_element_zone_main_item_add");
          uiItemO(add_remove_col,
                  "",
                  ICON_REMOVE,
                  "node.foreach_geometry_element_zone_main_item_remove");
        }
        {
          uiLayout *up_down_col = uiLayoutColumn(ops_col, true);
          uiItemEnumO(up_down_col,
                      "node.foreach_geometry_element_zone_main_item_move",
                      "",
                      ICON_TRIA_UP,
                      "direction",
                      0);
          uiItemEnumO(up_down_col,
                      "node.foreach_geometry_element_zone_main_item_move",
                      "",
                      ICON_TRIA_DOWN,
                      "direction",
                      1);
        }
      }

      if (storage.main_items.active_index >= 0 &&
          storage.main_items.active_index < storage.main_items.items_num)
      {
        NodeForeachGeometryElementMainItem &active_item =
            storage.main_items.items[storage.main_items.active_index];
        PointerRNA item_ptr = RNA_pointer_create(
            output_node_ptr.owner_id,
            ForeachGeometryElementMainItemsAccessor::item_srna,
            &active_item);
        uiLayoutSetPropSep(panel, true);
        uiLayoutSetPropDecorate(panel, false);
        uiItemR(panel, &item_ptr, "socket_type", UI_ITEM_NONE, nullptr, ICON_NONE);
      }
    }
    if (uiLayout *panel = uiLayoutPanel(
            C, layout, "generation_items", false, TIP_("Generated Geometry")))
    {
      static const uiListType *generation_items_list = []() {
        uiListType *list = MEM_cnew<uiListType>(__func__);
        STRNCPY(list->idname, "DATA_UL_foreach_geometry_element_generation_items");
        list->draw_item = draw_item;
        WM_uilisttype_add(list);
        return list;
      }();
      uiLayout *row = uiLayoutRow(panel, false);
      uiTemplateList(row,
                     C,
                     generation_items_list->idname,
                     "",
                     &output_node_ptr,
                     "generation_items",
                     &output_node_ptr,
                     "active_generation_index",
                     nullptr,
                     3,
                     5,
                     UILST_LAYOUT_DEFAULT,
                     0,
                     UI_TEMPLATE_LIST_FLAG_NONE);
      {
        uiLayout *ops_col = uiLayoutColumn(row, false);
        {
          uiLayout *add_remove_col = uiLayoutColumn(ops_col, true);
          uiItemO(add_remove_col,
                  "",
                  ICON_ADD,
                  "node.foreach_geometry_element_zone_generation_item_add");
          uiItemO(add_remove_col,
                  "",
                  ICON_REMOVE,
                  "node.foreach_geometry_element_zone_generation_item_remove");
        }
        {
          uiLayout *up_down_col = uiLayoutColumn(ops_col, true);
          uiItemEnumO(up_down_col,
                      "node.foreach_geometry_element_zone_generation_item_move",
                      "",
                      ICON_TRIA_UP,
                      "direction",
                      0);
          uiItemEnumO(up_down_col,
                      "node.foreach_geometry_element_zone_generation_item_move",
                      "",
                      ICON_TRIA_DOWN,
                      "direction",
                      1);
        }
      }

      if (storage.generation_items.active_index >= 0 &&
          storage.generation_items.active_index < storage.generation_items.items_num)
      {
        NodeForeachGeometryElementGenerationItem &active_item =
            storage.generation_items.items[storage.generation_items.active_index];
        PointerRNA item_ptr = RNA_pointer_create(
            output_node_ptr.owner_id,
            ForeachGeometryElementGenerationItemsAccessor::item_srna,
            &active_item);
        uiLayoutSetPropSep(panel, true);
        uiLayoutSetPropDecorate(panel, false);
        uiItemR(panel, &item_ptr, "socket_type", UI_ITEM_NONE, nullptr, ICON_NONE);
        if (active_item.socket_type != SOCK_GEOMETRY) {
          uiItemR(panel, &item_ptr, "domain", UI_ITEM_NONE, nullptr, ICON_NONE);
        }
      }
    }
  }

  uiItemR(layout, &output_node_ptr, "inspection_index", UI_ITEM_NONE, nullptr, ICON_NONE);
}

namespace input_node {

NODE_STORAGE_FUNCS(NodeGeometryForeachGeometryElementInput);

static void node_declare(NodeDeclarationBuilder &b)
{
  b.use_custom_socket_order();
  b.allow_any_socket_order();
  const bNode *node = b.node_or_null();
  const bNodeTree *tree = b.tree_or_null();

  if (!node || !tree) {
    return;
  }

  const NodeGeometryForeachGeometryElementInput &storage = node_storage(*node);
  const bNode *output_node = tree->node_by_id(storage.output_node_id);
  const auto &output_storage = output_node ?
                                   static_cast<const NodeGeometryForeachGeometryElementOutput *>(
                                       output_node->storage) :
                                   nullptr;

  b.add_output<decl::Int>("Index").description(
      "Index of the element in the source geometry. Note that the same index can occure more than "
      "once when iterating over multiple components at once");

  b.add_output<decl::Geometry>("Element")
      .description(
          "Single element geometry for the current iteration. Note that it can be quite "
          "inefficient to splitup large geometries into many small geometries")
      .propagate_all()
      .available(output_storage && AttrDomain(output_storage->domain) != AttrDomain::Corner);

  b.add_input<decl::Geometry>("Geometry").description("Geometry whose elements are iterated over");

  b.add_input<decl::Bool>("Selection")
      .default_value(true)
      .hide_value()
      .field_on_all()
      .description("Selection on the iteration domain");

  if (output_storage) {
    for (const int i : IndexRange(output_storage->input_items.items_num)) {
      const NodeForeachGeometryElementInputItem &item = output_storage->input_items.items[i];
      const eNodeSocketDatatype socket_type = eNodeSocketDatatype(item.socket_type);
      const StringRef name = item.name ? item.name : "";
      const std::string identifier =
          ForeachGeometryElementInputItemsAccessor::socket_identifier_for_item(item);
      auto &input_decl = b.add_input(socket_type, name, identifier)
                             .socket_name_ptr(&tree->id,
                                              ForeachGeometryElementInputItemsAccessor::item_srna,
                                              &item,
                                              "name")
                             .description("Field that is evaluated on the iteration domain");
      b.add_output(socket_type, name, identifier)
          .align_with_previous()
          .description("Evaluated field value for the current element");
      input_decl.supports_field();
    }
  }

  b.add_input<decl::Extend>("", "__extend__");
  b.add_output<decl::Extend>("", "__extend__").align_with_previous();
}

static void node_layout(uiLayout *layout, bContext * /*C*/, PointerRNA *ptr)
{
  bNodeTree &tree = *reinterpret_cast<bNodeTree *>(ptr->owner_id);
  bNode &node = *static_cast<bNode *>(ptr->data);
  const NodeGeometryForeachGeometryElementInput &storage = node_storage(node);
  bNode *output_node = tree.node_by_id(storage.output_node_id);

  PointerRNA output_node_ptr = RNA_pointer_create(ptr->owner_id, &RNA_Node, output_node);
  uiItemR(layout, &output_node_ptr, "domain", UI_ITEM_NONE, "", ICON_NONE);
}

static void node_init(bNodeTree * /*tree*/, bNode *node)
{
  NodeGeometryForeachGeometryElementInput *data =
      MEM_cnew<NodeGeometryForeachGeometryElementInput>(__func__);
  /* Needs to be initialized for the node to work. */
  data->output_node_id = 0;
  node->storage = data;
}

static void node_label(const bNodeTree * /*ntree*/,
                       const bNode * /*node*/,
                       char *label,
                       const int label_maxncpy)
{
  BLI_strncpy_utf8(label, IFACE_("For Each Element"), label_maxncpy);
}

static bool node_insert_link(bNodeTree *ntree, bNode *node, bNodeLink *link)
{
  bNode *output_node = ntree->node_by_id(node_storage(*node).output_node_id);
  if (!output_node) {
    return true;
  }
  return socket_items::try_add_item_via_any_extend_socket<
      ForeachGeometryElementInputItemsAccessor>(*ntree, *node, *output_node, *link);
}

static void node_register()
{
  static blender::bke::bNodeType ntype;
  geo_node_type_base(&ntype,
                     GEO_NODE_FOREACH_GEOMETRY_ELEMENT_INPUT,
                     "For Each Geometry Element Input",
                     NODE_CLASS_INTERFACE);
  ntype.initfunc = node_init;
  ntype.declare = node_declare;
  ntype.draw_buttons = node_layout;
  ntype.draw_buttons_ex = node_layout_ex;
  ntype.labelfunc = node_label;
  ntype.insert_link = node_insert_link;
  ntype.gather_link_search_ops = nullptr;
  ntype.no_muting = true;
  blender::bke::node_type_storage(&ntype,
                                  "NodeGeometryForeachGeometryElementInput",
                                  node_free_standard_storage,
                                  node_copy_standard_storage);
  blender::bke::node_register_type(&ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace input_node

namespace output_node {

NODE_STORAGE_FUNCS(NodeGeometryForeachGeometryElementOutput);

static void node_declare(NodeDeclarationBuilder &b)
{
  b.use_custom_socket_order();
  b.allow_any_socket_order();

  b.add_output<decl::Geometry>("Geometry")
      .description(
          "The original input geometry with potentially new attributes that are output by the "
          "zone");

  aal::RelationsInNode &relations = b.get_anonymous_attribute_relations();

  const bNode *node = b.node_or_null();
  const bNodeTree *tree = b.tree_or_null();
  if (node && tree) {
    const NodeGeometryForeachGeometryElementOutput &storage = node_storage(*node);
    for (const int i : IndexRange(storage.main_items.items_num)) {
      const NodeForeachGeometryElementMainItem &item = storage.main_items.items[i];
      const eNodeSocketDatatype socket_type = eNodeSocketDatatype(item.socket_type);
      const StringRef name = item.name ? item.name : "";
      std::string identifier = ForeachGeometryElementMainItemsAccessor::socket_identifier_for_item(
          item);
      b.add_input(socket_type, name, identifier)
          .socket_name_ptr(
              &tree->id, ForeachGeometryElementMainItemsAccessor::item_srna, &item, "name")
          .description(
              "Attribute value that will be stored for the current element on the main geometry");
      b.add_output(socket_type, name, identifier)
          .align_with_previous()
          .field_on({0})
          .description("Attribute on the geometry above");
    }
    b.add_input<decl::Extend>("", "__extend__main");
    b.add_output<decl::Extend>("", "__extend__main").align_with_previous();
    b.add_separator();
    int previous_geometry_index = -1;
    for (const int i : IndexRange(storage.generation_items.items_num)) {
      const NodeForeachGeometryElementGenerationItem &item = storage.generation_items.items[i];
      const eNodeSocketDatatype socket_type = eNodeSocketDatatype(item.socket_type);
      if (socket_type == SOCK_GEOMETRY && i > 0) {
        b.add_separator();
      }
      const StringRef name = item.name ? item.name : "";
      std::string identifier =
          ForeachGeometryElementGenerationItemsAccessor::socket_identifier_for_item(item);
      auto &input_decl = b.add_input(socket_type, name, identifier)
                             .socket_name_ptr(
                                 &tree->id,
                                 ForeachGeometryElementGenerationItemsAccessor::item_srna,
                                 &item,
                                 "name");
      auto &output_decl = b.add_output(socket_type, name, identifier).align_with_previous();
      if (socket_type == SOCK_GEOMETRY) {
        previous_geometry_index = output_decl.index();
        aal::PropagateRelation relation;
        relation.from_geometry_input = input_decl.index();
        relation.to_geometry_output = output_decl.index();
        relations.propagate_relations.append(relation);

        input_decl.description(
            "Geometry generated in the current iteration. Will be joined with geometries from all "
            "other iterations");
        output_decl.description("Result of joining generated geometries from each iteration");
      }
      else {
        input_decl.supports_field();
        if (previous_geometry_index > 0) {
          input_decl.description("Field that will be stored as attribute on the geometry above");
          output_decl.field_on({previous_geometry_index});
        }
        output_decl.description("Attribute on the geometry above");
      }
    }
  }

  b.add_input<decl::Extend>("", "__extend__generation");
  b.add_output<decl::Extend>("", "__extend__generation").align_with_previous();
}

static void node_init(bNodeTree * /*tree*/, bNode *node)
{
  NodeGeometryForeachGeometryElementOutput *data =
      MEM_cnew<NodeGeometryForeachGeometryElementOutput>(__func__);

  data->generation_items.items = MEM_cnew_array<NodeForeachGeometryElementGenerationItem>(
      1, __func__);
  NodeForeachGeometryElementGenerationItem &item = data->generation_items.items[0];
  item.name = BLI_strdup(DATA_("Geometry"));
  item.socket_type = SOCK_GEOMETRY;
  item.identifier = data->generation_items.next_identifier++;
  data->generation_items.items_num = 1;

  node->storage = data;
}

static void node_free_storage(bNode *node)
{
  socket_items::destruct_array<ForeachGeometryElementInputItemsAccessor>(*node);
  socket_items::destruct_array<ForeachGeometryElementGenerationItemsAccessor>(*node);
  socket_items::destruct_array<ForeachGeometryElementMainItemsAccessor>(*node);
  MEM_freeN(node->storage);
}

static void node_copy_storage(bNodeTree * /*dst_tree*/, bNode *dst_node, const bNode *src_node)
{
  const NodeGeometryForeachGeometryElementOutput &src_storage = node_storage(*src_node);
  auto *dst_storage = MEM_cnew<NodeGeometryForeachGeometryElementOutput>(__func__, src_storage);
  dst_node->storage = dst_storage;

  socket_items::copy_array<ForeachGeometryElementInputItemsAccessor>(*src_node, *dst_node);
  socket_items::copy_array<ForeachGeometryElementGenerationItemsAccessor>(*src_node, *dst_node);
  socket_items::copy_array<ForeachGeometryElementMainItemsAccessor>(*src_node, *dst_node);
}

static bool node_insert_link(bNodeTree *ntree, bNode *node, bNodeLink *link)
{
  if (!socket_items::try_add_item_via_any_extend_socket<ForeachGeometryElementMainItemsAccessor>(
          *ntree, *node, *node, *link, "__extend__main"))
  {
    return false;
  }
  return socket_items::try_add_item_via_any_extend_socket<
      ForeachGeometryElementGenerationItemsAccessor>(*ntree, *node, *node, *link);
}

static void NODE_OT_foreach_geometry_element_zone_input_item_remove(wmOperatorType *ot)
{
  socket_items::ops::remove_active_item<ForeachGeometryElementInputItemsAccessor>(
      ot, "Remove For Each Input Item", __func__, "Remove active for-each input item");
}

static void NODE_OT_foreach_geometry_element_zone_input_item_add(wmOperatorType *ot)
{
  socket_items::ops::add_item<ForeachGeometryElementInputItemsAccessor>(
      ot, "Add For Each Input Item", __func__, "Add for-each input item");
}

static void NODE_OT_foreach_geometry_element_zone_input_item_move(wmOperatorType *ot)
{
  socket_items::ops::move_active_item<ForeachGeometryElementInputItemsAccessor>(
      ot, "Move For Each Input Item", __func__, "Move active for-each input item");
}

static void NODE_OT_foreach_geometry_element_zone_generation_item_remove(wmOperatorType *ot)
{
  socket_items::ops::remove_active_item<ForeachGeometryElementGenerationItemsAccessor>(
      ot, "Remove For Each Generation Item", __func__, "Remove active for-each generation item");
}

static void NODE_OT_foreach_geometry_element_zone_generation_item_add(wmOperatorType *ot)
{
  socket_items::ops::add_item<ForeachGeometryElementGenerationItemsAccessor>(
      ot, "Add For Each Generation Item", __func__, "Add for-each generation item");
}

static void NODE_OT_foreach_geometry_element_zone_generation_item_move(wmOperatorType *ot)
{
  socket_items::ops::move_active_item<ForeachGeometryElementGenerationItemsAccessor>(
      ot, "Move For Each Generation Item", __func__, "Move active for-each generation item");
}

static void NODE_OT_foreach_geometry_element_zone_main_item_remove(wmOperatorType *ot)
{
  socket_items::ops::remove_active_item<ForeachGeometryElementMainItemsAccessor>(
      ot, "Remove For Each Main Item", __func__, "Remove active for-each main item");
}

static void NODE_OT_foreach_geometry_element_zone_main_item_add(wmOperatorType *ot)
{
  socket_items::ops::add_item<ForeachGeometryElementMainItemsAccessor>(
      ot, "Add For Each Main Item", __func__, "Add for-each main item");
}

static void NODE_OT_foreach_geometry_element_zone_main_item_move(wmOperatorType *ot)
{
  socket_items::ops::move_active_item<ForeachGeometryElementMainItemsAccessor>(
      ot, "Move For Each Main Item", __func__, "Move active for-each main item");
}

static void node_operators()
{
  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_input_item_add);
  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_input_item_remove);
  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_input_item_move);

  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_generation_item_add);
  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_generation_item_remove);
  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_generation_item_move);

  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_main_item_add);
  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_main_item_remove);
  WM_operatortype_append(NODE_OT_foreach_geometry_element_zone_main_item_move);
}

static void node_extra_info(NodeExtraInfoParams &params)
{
  const NodeGeometryForeachGeometryElementOutput &storage = node_storage(params.node);
  if (storage.generation_items.items_num > 0) {
    if (storage.generation_items.items[0].socket_type != SOCK_GEOMETRY) {
      NodeExtraInfoRow row;
      row.text = RPT_("Missing Geometry");
      row.tooltip = TIP_("Each output field has to correspond to a geometry that is above it");
      row.icon = ICON_ERROR;
      params.rows.append(std::move(row));
    }
  }
}

static void node_register()
{
  static blender::bke::bNodeType ntype;
  geo_node_type_base(&ntype,
                     GEO_NODE_FOREACH_GEOMETRY_ELEMENT_OUTPUT,
                     "For Each Geometry Element Output",
                     NODE_CLASS_INTERFACE);
  ntype.initfunc = node_init;
  ntype.declare = node_declare;
  ntype.labelfunc = input_node::node_label;
  ntype.insert_link = node_insert_link;
  ntype.draw_buttons_ex = node_layout_ex;
  ntype.register_operators = node_operators;
  ntype.get_extra_info = node_extra_info;
  ntype.no_muting = true;
  blender::bke::node_type_storage(
      &ntype, "NodeGeometryForeachGeometryElementOutput", node_free_storage, node_copy_storage);
  blender::bke::node_register_type(&ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace output_node

}  // namespace blender::nodes::node_geo_foreach_geometry_element_cc

namespace blender::nodes {

StructRNA *ForeachGeometryElementInputItemsAccessor::item_srna =
    &RNA_ForeachGeometryElementInputItem;
int ForeachGeometryElementInputItemsAccessor::node_type = GEO_NODE_FOREACH_GEOMETRY_ELEMENT_OUTPUT;

void ForeachGeometryElementInputItemsAccessor::blend_write(BlendWriter *writer, const bNode &node)
{
  const auto &storage = *static_cast<const NodeGeometryForeachGeometryElementOutput *>(
      node.storage);
  BLO_write_struct_array(writer,
                         NodeForeachGeometryElementInputItem,
                         storage.input_items.items_num,
                         storage.input_items.items);
  for (const NodeForeachGeometryElementInputItem &item :
       Span(storage.input_items.items, storage.input_items.items_num))
  {
    BLO_write_string(writer, item.name);
  }
}

void ForeachGeometryElementInputItemsAccessor::blend_read_data(BlendDataReader *reader,
                                                               bNode &node)
{
  auto &storage = *static_cast<NodeGeometryForeachGeometryElementOutput *>(node.storage);
  BLO_read_struct_array(reader,
                        NodeForeachGeometryElementInputItem,
                        storage.input_items.items_num,
                        &storage.input_items.items);
  for (const NodeForeachGeometryElementInputItem &item :
       Span(storage.input_items.items, storage.input_items.items_num))
  {
    BLO_read_string(reader, &item.name);
  }
}

StructRNA *ForeachGeometryElementMainItemsAccessor::item_srna =
    &RNA_ForeachGeometryElementMainItem;
int ForeachGeometryElementMainItemsAccessor::node_type = GEO_NODE_FOREACH_GEOMETRY_ELEMENT_OUTPUT;

void ForeachGeometryElementMainItemsAccessor::blend_write(BlendWriter *writer, const bNode &node)
{
  const auto &storage = *static_cast<const NodeGeometryForeachGeometryElementOutput *>(
      node.storage);
  BLO_write_struct_array(writer,
                         NodeForeachGeometryElementMainItem,
                         storage.main_items.items_num,
                         storage.main_items.items);
  for (const NodeForeachGeometryElementMainItem &item :
       Span(storage.main_items.items, storage.main_items.items_num))
  {
    BLO_write_string(writer, item.name);
  }
}

void ForeachGeometryElementMainItemsAccessor::blend_read_data(BlendDataReader *reader, bNode &node)
{
  auto &storage = *static_cast<NodeGeometryForeachGeometryElementOutput *>(node.storage);
  BLO_read_struct_array(reader,
                        NodeForeachGeometryElementMainItem,
                        storage.main_items.items_num,
                        &storage.main_items.items);
  for (const NodeForeachGeometryElementMainItem &item :
       Span(storage.main_items.items, storage.main_items.items_num))
  {
    BLO_read_string(reader, &item.name);
  }
}

StructRNA *ForeachGeometryElementGenerationItemsAccessor::item_srna =
    &RNA_ForeachGeometryElementGenerationItem;
int ForeachGeometryElementGenerationItemsAccessor::node_type =
    GEO_NODE_FOREACH_GEOMETRY_ELEMENT_OUTPUT;

void ForeachGeometryElementGenerationItemsAccessor::blend_write(BlendWriter *writer,
                                                                const bNode &node)
{
  const auto &storage = *static_cast<const NodeGeometryForeachGeometryElementOutput *>(
      node.storage);
  BLO_write_struct_array(writer,
                         NodeForeachGeometryElementGenerationItem,
                         storage.generation_items.items_num,
                         storage.generation_items.items);
  for (const NodeForeachGeometryElementGenerationItem &item :
       Span(storage.generation_items.items, storage.generation_items.items_num))
  {
    BLO_write_string(writer, item.name);
  }
}

void ForeachGeometryElementGenerationItemsAccessor::blend_read_data(BlendDataReader *reader,
                                                                    bNode &node)
{
  auto &storage = *static_cast<NodeGeometryForeachGeometryElementOutput *>(node.storage);
  BLO_read_struct_array(reader,
                        NodeForeachGeometryElementGenerationItem,
                        storage.generation_items.items_num,
                        &storage.generation_items.items);
  for (const NodeForeachGeometryElementGenerationItem &item :
       Span(storage.generation_items.items, storage.generation_items.items_num))
  {
    BLO_read_string(reader, &item.name);
  }
}

}  // namespace blender::nodes
