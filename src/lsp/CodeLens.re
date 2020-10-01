open Infix;

let sepList = items =>
  List.length(items) <= 4
    ? String.concat(", ", items)
    : String.concat(", ", Belt.List.take(items, 3) |? [])
      ++ " and "
      ++ string_of_int(List.length(items) - 3)
      ++ " more";

let forOpen = (tracker: SharedTypes.openTracker) => {
  let items =
    tracker.used
    |> List.map(((path, tip, _loc)) =>
         switch (path) {
         | SharedTypes.Tip(name) => (name, tip)
         | Nested(name, _) => (name, SharedTypes.Module)
         }
       )
    |> List.sort_uniq(compare);
  let values = items |> List.filter(((_, t)) => t == SharedTypes.Value);
  let modules = items |> List.filter(((_, t)) => t == SharedTypes.Module);
  let types =
    items |> List.filter(((_, t)) => t != SharedTypes.Value && t != Module);

  let typeMap = Hashtbl.create(10);
  List.iter(
    ((name, t)) => {
      let current =
        Hashtbl.mem(typeMap, name) ? Hashtbl.find(typeMap, name) : [];
      let current =
        switch (t) {
        | SharedTypes.Constructor(name) => [name, ...current]
        | Attribute(name) => [name, ...current]
        | _ => current
        };
      Hashtbl.replace(typeMap, name, current);
    },
    types,
  );

  (items, types, modules, values, typeMap);
};

let forOpens = (extra: SharedTypes.extra) => {
  SharedTypes.hashList(extra.opens)
  |> List.map(((_loc, tracker)) => {
       let (items, types, modules, values, typeMap) = forOpen(tracker);

       let parts = [];
       let parts =
         types == []
           ? parts
           : [
             "types: {"
             ++ String.concat(
                  ", ",
                  Hashtbl.fold(
                    (t, items, res) =>
                      [
                        items == [] ? t : t ++ " [" ++ sepList(items) ++ "]",
                        ...res,
                      ],
                    typeMap,
                    [],
                  ),
                )
             ++ "}",
             ...parts,
           ];
       let parts =
         modules == []
           ? parts
           : [
             "modules: {"
             ++ String.concat(", ", List.map(fst, modules))
             ++ "}",
             ...parts,
           ];
       let parts =
         values == []
           ? parts
           : [
             "values: {" ++ String.concat(", ", List.map(fst, values)) ++ "}",
             ...parts,
           ];

       (
         parts == []
           ? "Unused open"
           : string_of_int(List.length(items))
             ++ " uses. "
             ++ String.concat(" ", parts),
         tracker.ident.loc,
       );
     });
};
