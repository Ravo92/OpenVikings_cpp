"""Desktop editor for Windows Cultures: 8th Wonder of the World saves.

Run with Python 3.10+ and Tkinter. No third-party packages are required.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, ttk

from catalog import Catalog
from map_export import MapExport
from savegame import CHUNK_NAMES, HOUSE_FIELDS, HOUSES, HUMAN_FIELDS, HUMANS, SaveError, Savegame, inspect_file, u32


ERRORS = (OSError, ValueError, KeyError, struct.error)


def table(parent, columns, widths=None):
    frame = ttk.Frame(parent)
    frame.pack(fill="both", expand=True)
    tree = ttk.Treeview(frame, columns=columns, show="headings", selectmode="browse")
    for index, column in enumerate(columns):
        tree.heading(column, text=column)
        tree.column(column, width=(widths or [110] * len(columns))[index], minwidth=45)
    scroll = ttk.Scrollbar(frame, orient="vertical", command=tree.yview)
    horizontal = ttk.Scrollbar(frame, orient="horizontal", command=tree.xview)
    tree.configure(yscrollcommand=scroll.set, xscrollcommand=horizontal.set)
    tree.grid(row=0, column=0, sticky="nsew")
    scroll.grid(row=0, column=1, sticky="ns")
    horizontal.grid(row=1, column=0, sticky="ew")
    frame.rowconfigure(0, weight=1)
    frame.columnconfigure(0, weight=1)
    return tree


class RecordTab:
    def __init__(self, app, notebook, marker, title, fields):
        self.app, self.marker, self.fields = app, marker, fields
        self.current, self.loading = None, False
        self.frame = ttk.Frame(notebook, padding=12)
        notebook.add(self.frame, text=title)
        top = ttk.Frame(self.frame)
        top.pack(fill="x", pady=(0, 10))
        ttk.Label(top, text="Filter by ID, name, player or type").pack(side="left")
        self.search = tk.StringVar()
        ttk.Entry(top, textvariable=self.search, width=40).pack(side="left", padx=12)
        ttk.Button(top, text="Filter", command=self.filter).pack(side="left")
        panes = ttk.Panedwindow(self.frame, orient="horizontal")
        panes.pack(fill="both", expand=True)
        left, details = ttk.Frame(panes), ttk.Frame(panes)
        panes.add(left, weight=3)
        panes.add(details, weight=1)
        canvas = tk.Canvas(details, width=350, highlightthickness=0, background=ttk.Style().lookup("TFrame", "background"))
        detail_scroll = ttk.Scrollbar(details, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=detail_scroll.set)
        detail_scroll.pack(side="right", fill="y")
        canvas.pack(side="left", fill="both", expand=True)
        right = ttk.Frame(canvas, padding=(16, 0))
        detail_window = canvas.create_window((0, 0), window=right, anchor="nw")
        right.bind("<Configure>", lambda event: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind("<Configure>", lambda event: canvas.itemconfigure(detail_window, width=event.width))
        columns = ("ID", "Name", "Player", "Job", "Position") if marker == HUMANS else ("ID", "Building", "Player", "Position", "Stage")
        self.tree = table(left, columns, [60, 190, 65, 150, 100])
        self.tree.bind("<<TreeviewSelect>>", self.select)
        self.search_entry = top.winfo_children()[1]
        self.search_entry.bind("<Return>", lambda event: self.filter())
        self.heading = tk.StringVar(value="Select an object")
        ttk.Label(right, textvariable=self.heading, font=("Segoe UI", 13, "bold"), wraplength=310).pack(anchor="w", pady=(0, 12))
        self.variables = {}
        self.entries = []
        if marker == HUMANS:
            self.add_field(right, "name", "Name (up to 99 UTF-8 bytes; Unicode needs Remaster)")
        for field in fields:
            self.add_field(right, field.key, field.label)
        ttk.Button(right, text="Apply values", command=self.apply_clicked).pack(fill="x", pady=(12, 4))
        if marker == HUMANS:
            ttk.Button(right, text="Refill selected villager's needs", command=self.refill).pack(fill="x", pady=4)
            ttk.Button(right, text="Edit job experience...", command=self.experience).pack(fill="x", pady=4)
        else:
            ttk.Button(right, text="Edit stored goods...", command=self.stocks).pack(fill="x", pady=4)
        note = "Changes apply when you select another row or save. Job, owner, position and family links are shown without editing their dependent state." if marker == HUMANS else "Stock slots are mapped using your game definitions. Construction state, ownership and incoming deliveries stay unchanged."
        ttk.Label(right, text=note, wraplength=310, foreground="#546174").pack(anchor="w", pady=14)

    def add_field(self, parent, key, label):
        ttk.Label(parent, text=label, wraplength=310).pack(anchor="w", pady=(5, 2))
        variable = tk.StringVar()
        entry = ttk.Entry(parent, textvariable=variable)
        entry.pack(fill="x")
        self.variables[key] = variable
        self.entries.append(entry)

    def rows(self):
        save = self.app.save
        if save is None or self.marker not in save.arrays:
            return []
        array = save.arrays[self.marker]
        result = []
        for index in array.records:
            record = array.record(index)
            if self.marker == HUMANS:
                result.append((index, save.name(index), u32(record, 0x10), self.app.catalog.job_name(u32(record, 0x18)), save.position(record)))
            else:
                x, y = struct.unpack_from("<2h", record, 0x1C)
                result.append((index, self.app.catalog.house_name(u32(record, 0x14)), u32(record, 12), f"{x}, {y}", u32(record, 0x20)))
        return result

    def refresh(self, preserve=True):
        previous = self.current if preserve else None
        self.loading = True
        self.tree.delete(*self.tree.get_children())
        query = self.search.get().casefold()
        for row in self.rows():
            if not query or query in " ".join(map(str, row)).casefold():
                self.tree.insert("", "end", iid=str(row[0]), values=row)
        self.current = previous if previous is not None and self.tree.exists(str(previous)) else None
        if self.current is not None:
            self.tree.selection_set(str(self.current))
        self.load_form()
        self.loading = False

    def load_form(self):
        if self.current is None or self.app.save is None:
            self.heading.set("Select an object")
            for variable in self.variables.values():
                variable.set("")
            return
        save = self.app.save
        record = save.arrays[self.marker].record(self.current)
        self.heading.set(f"{save.name(self.current) if self.marker == HUMANS else 'Building'} · #{self.current}")
        for field in self.fields:
            self.variables[field.key].set(str(u32(record, field.offset)))
        if self.marker == HUMANS:
            self.variables["name"].set(save.name(self.current))
            self.entries[0].configure(state="normal" if self.current in save.names else "readonly")

    def apply(self):
        if self.current is None or self.app.save is None:
            return
        save = self.app.save
        record = save.arrays[self.marker].record(self.current)
        values = {}
        for field in self.fields:
            value = int(self.variables[field.key].get())
            if value != u32(record, field.offset):
                values[field.key] = value
        name = None
        if self.marker == HUMANS and self.current in save.names and self.variables["name"].get() != save.name(self.current):
            name = self.variables["name"].get()
        if values or name is not None:
            save.edit_record(self.marker, self.current, values, name)
        self.app.status()

    def apply_clicked(self):
        self.app.run_action(lambda: (self.apply(), self.refresh()))

    def select(self, event=None):
        if self.loading:
            return
        selection = self.tree.selection()
        if not selection or int(selection[0]) == self.current:
            return
        try:
            self.apply()
        except ERRORS as error:
            messagebox.showerror("Invalid value", str(error), parent=self.app.root)
            if self.current is not None and self.tree.exists(str(self.current)):
                self.tree.selection_set(str(self.current))
            return
        self.current = int(selection[0])
        self.load_form()

    def filter(self):
        self.app.run_action(lambda: (self.apply(), self.refresh()))

    def refill(self):
        def action():
            self.apply()
            if self.current is not None:
                self.app.save.edit_record(HUMANS, self.current, {key: 10000 for key in ("food", "stamina", "social", "religion")})
                self.load_form()
                self.app.status()
        self.app.run_action(action)

    def experience(self):
        self.app.run_action(self._experience)

    def _experience(self):
        self.apply()
        if self.current is None:
            return
        index, save = self.current, self.app.save
        window = self.app.dialog(f"Job experience · {save.name(index)}", "700x560")
        ttk.Label(window, text="Double-click a slot to edit it. Values range from 0 to 10000.").pack(anchor="w", pady=(0, 10))
        tree = table(window, ("Slot", "Experience type", "Experience"), [60, 350, 130])

        def refresh():
            tree.delete(*tree.get_children())
            record = save.arrays[HUMANS].record(index)
            for slot in range(78):
                tree.insert("", "end", iid=str(slot), values=(slot, self.app.catalog.experience_name(slot), u32(record, 0x24 + slot * 4)))

        def edit(event=None):
            selected = tree.selection()
            if not selected:
                return
            slot = int(selected[0])
            old = u32(save.arrays[HUMANS].record(index), 0x24 + slot * 4)
            value = simpledialog.askinteger("Job experience", f"Slot {slot}: {self.app.catalog.experience_name(slot)}", initialvalue=old, minvalue=0, maxvalue=10000, parent=window.winfo_toplevel())
            if value is not None:
                self.app.run_action(lambda: save.edit_experience(index, slot, value))
                refresh()
            window.winfo_toplevel().grab_set()

        tree.bind("<Double-1>", edit)
        ttk.Button(window, text="Edit selected slot", command=edit).pack(fill="x", pady=(10, 0))
        refresh()

    def stocks(self):
        self.app.run_action(self._stocks)

    def _stocks(self):
        self.apply()
        if self.current is None:
            return
        save, index = self.app.save, self.current
        record = save.arrays[HOUSES].record(index)
        if u32(record, 0x2C) != 3:
            raise SaveError("Stock editing is available for completed buildings. Construction materials use a separate structure.")
        definition = self.app.catalog.houses.get(u32(record, 0x14))
        if definition is None:
            raise SaveError("Load your game data first using 'Game data...'. Stock slots depend on the building definition.")
        stocks = definition["stocks"]
        window = self.app.dialog(f"Stored goods · {definition['name']} #{index}", "810x550")
        ttk.Label(window, text="Double-click a stock to edit physical inventory. Tenths are preserved unless edited.").pack(anchor="w", pady=(0, 10))
        ttk.Label(window, text="Capacity is the base game-data value; the scenario may apply a different storage mode.", foreground="#546174").pack(anchor="w", pady=(0, 8))
        tree = table(window, ("Slot", "Good", "Whole", "Tenths", "Base capacity", "Output"), [55, 230, 80, 80, 110, 70])

        def refresh():
            tree.delete(*tree.get_children())
            data = save.arrays[HOUSES].record(index)
            for slot, (good, maximum, output) in enumerate(stocks):
                tree.insert("", "end", iid=str(slot), values=(slot, f"{self.app.catalog.good_name(good)} ({good})", u32(data, 0x118 + slot * 4), u32(data, 0x220 + slot * 4), maximum, "Yes" if output else "No"))

        def edit(event=None):
            selected = tree.selection()
            if not selected:
                return
            slot = int(selected[0])
            good, maximum, output = stocks[slot]
            data = save.arrays[HOUSES].record(index)
            # Editor bounds follow the game's hard physical-stock limit. A base
            # capacity is displayed separately because scenario modes can vary it.
            amount = simpledialog.askinteger("Stored goods", f"{self.app.catalog.good_name(good)}: whole units (0-1000)", initialvalue=u32(data, 0x118 + slot * 4), minvalue=0, maxvalue=1000, parent=window.winfo_toplevel())
            if amount is None:
                window.winfo_toplevel().grab_set()
                return
            fraction = 0 if amount == 1000 else simpledialog.askinteger("Stored goods", "Tenths of one unit (0-9)", initialvalue=u32(data, 0x220 + slot * 4), minvalue=0, maxvalue=9, parent=window.winfo_toplevel())
            if fraction is not None:
                self.app.run_action(lambda: save.edit_stock(index, slot, amount, fraction, 1000))
                refresh()
            window.winfo_toplevel().grab_set()

        tree.bind("<Double-1>", edit)
        ttk.Button(window, text="Edit selected stock", command=edit).pack(fill="x", pady=(10, 0))
        refresh()


class Editor:
    def __init__(self, root):
        self.root, self.save, self.catalog = root, None, Catalog()
        self.saved_bytes = None
        self.dialogs = []
        root.title("OpenVikings Savegame Editor")
        root.geometry("1240x800")
        root.minsize(1050, 700)
        style = ttk.Style(root)
        if "vista" in style.theme_names():
            style.theme_use("vista")
        root.option_add("*Font", "{Segoe UI} 10")
        style.configure("Treeview", rowheight=27)
        style.configure("Treeview.Heading", font=("Segoe UI", 10, "bold"))
        toolbar = ttk.Frame(root, padding=12)
        toolbar.pack(fill="x")
        ttk.Label(toolbar, text="OpenVikings", font=("Segoe UI", 17, "bold")).pack(side="left", padx=(0, 20))
        for title, command in (("Open save...", self.open_dialog), ("Save copy...", self.save_dialog), ("Undo", self.undo), ("Redo", self.redo), ("Game data...", self.game_data), ("Export JSON...", self.export), ("Export map...", self.export_map)):
            ttk.Button(toolbar, text=title, command=command).pack(side="left", padx=3)
        self.file_label = tk.StringVar(value="Open a .sav file to begin. The source save is never overwritten.")
        ttk.Label(root, textvariable=self.file_label, padding=(16, 0, 16, 10), foreground="#546174").pack(fill="x")
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill="both", expand=True, padx=12)
        self.info_frame = ttk.Frame(self.notebook, padding=20)
        self.notebook.add(self.info_frame, text="Overview")
        ttk.Label(self.info_frame, text="Save information", font=("Segoe UI", 16, "bold")).pack(anchor="w", pady=(0, 14))
        self.metadata_vars, self.metadata_entries = {}, {}
        for key, label in (("map_name", "Map display name"), ("description", "Save description"), ("slot", "Save slot ID")):
            ttk.Label(self.info_frame, text=label).pack(anchor="w", pady=(7, 3))
            variable = tk.StringVar()
            entry = ttk.Entry(self.info_frame, textvariable=variable, width=90)
            entry.pack(anchor="w", fill="x")
            self.metadata_vars[key], self.metadata_entries[key] = variable, entry
        ttk.Button(self.info_frame, text="Apply save information", command=lambda: self.run_action(self.apply_metadata)).pack(anchor="w", pady=12)
        self.summary = tk.StringVar(value="Windows Cultures: 8th Wonder of the World\n\nEdit existing villagers and completed building stocks, then save a new .sav copy.\nLoad your game's data folder for building, job and good names.")
        ttk.Label(self.info_frame, textvariable=self.summary, justify="left", wraplength=1030).pack(anchor="w", pady=12)
        self.tabs = [RecordTab(self, self.notebook, HUMANS, "Villagers", HUMAN_FIELDS), RecordTab(self, self.notebook, HOUSES, "Buildings", HOUSE_FIELDS)]
        block_frame = ttk.Frame(self.notebook, padding=12)
        self.notebook.add(block_frame, text="Original file blocks")
        self.blocks = table(block_frame, ("Tag", "Meaning", "Version", "Bytes", "Source offset"), [90, 340, 90, 120, 120])
        self.status_text = tk.StringVar(value="Ready")
        ttk.Label(root, textvariable=self.status_text, padding=12).pack(fill="x")
        root.protocol("WM_DELETE_WINDOW", self.close)
        root.bind("<Control-o>", lambda event: self.open_dialog())
        root.bind("<Control-s>", lambda event: self.save_dialog())

    def run_action(self, action):
        try:
            action()
            self.status()
        except ERRORS as error:
            messagebox.showerror("Savegame Editor", str(error), parent=self.root)

    def dialog(self, title, geometry):
        window = tk.Toplevel(self.root)
        window.title(title)
        window.geometry(geometry)
        window.transient(self.root)
        ttk.Frame(window, padding=12).pack(fill="both", expand=True)
        self.dialogs.append(window)
        # Modal editing prevents an undo/open from invalidating a child window.
        window.grab_set()
        return window.winfo_children()[0]

    def close_dialogs(self):
        for window in self.dialogs:
            if window.winfo_exists():
                window.destroy()
        self.dialogs.clear()

    def status(self):
        if self.save:
            count = len(self.save.history)
            self.status_text.set(f"{count} applied edit(s) · {len(self.save.chunks)} blocks · Game data: {self.catalog.root or 'not loaded'}")

    def apply_metadata(self):
        if self.save is None:
            return
        existing = self.save.metadata()
        changes = {key: variable.get() for key, variable in self.metadata_vars.items() if existing.get(key) is not None and variable.get() != str(existing[key])}
        if changes:
            self.save.edit_metadata(changes)
        self.status()

    def flush_forms(self):
        self.apply_metadata()
        for tab in self.tabs:
            tab.apply()

    def discard_ok(self):
        if self.save is None:
            return True
        self.flush_forms()
        return self.save.encode() == self.saved_bytes or messagebox.askyesno("Discard edits?", "There are changes that have not been saved to a copy. Discard them?", parent=self.root)

    def open_dialog(self):
        path = filedialog.askopenfilename(title="Open Cultures savegame", filetypes=[("Cultures saves", "*.sav"), ("All files", "*.*")], parent=self.root)
        if path:
            self.run_action(lambda: self.load(Path(path)))

    def load(self, path):
        new_save = Savegame.open(path)
        if not self.discard_ok():
            return
        self.close_dialogs()
        self.save = new_save
        self.saved_bytes = new_save.original
        for parent in path.resolve().parents:
            if (parent / "DataX" / "libs").is_dir():
                try:
                    self.catalog.load(parent)
                except ERRORS as error:
                    new_save.warnings.append(f"Optional game definitions were not loaded: {error}")
                break
        self.refresh()

    def refresh(self):
        if self.save is None:
            return
        save = self.save
        self.file_label.set(str(save.path))
        self.root.title(f"OpenVikings Savegame Editor · {save.path.name if save.path else 'Save'}")
        metadata = save.metadata()
        for key, variable in self.metadata_vars.items():
            value = metadata.get(key)
            variable.set(str(value) if value is not None else "")
            self.metadata_entries[key].configure(state="normal" if value is not None else "disabled")
        counts = "\n".join(f"{label}: {len(save.arrays[marker].records)}" for marker, label in ((HUMANS, "Villagers"), (HOUSES, "Buildings"), (0x78646161, "Animals"), (0x78647661, "Vehicles")) if marker in save.arrays)
        time = metadata.get("creation_time_fields", [])
        time_text = f"{time[0]:04}-{time[1]:02}-{time[2]:02} {time[3]:02}:{time[4]:02}:{time[5]:02}" if len(time) == 7 else "Unknown"
        self.summary.set(f"Saved: {time_text}\n{counts}\n\nMap display-name changes do not change the actual map or its GUID. Slot IDs affect menu ordering; special system saves also depend on their filenames.\n\nChanges are reversible with Undo. 'Save copy' creates a new file and refuses to overwrite existing files. Unknown blocks and untouched compressed data are preserved.\n\n" + "\n".join(save.warnings))
        for tab in self.tabs:
            tab.refresh()
        self.blocks.delete(*self.blocks.get_children())
        for chunk in save.chunks:
            self.blocks.insert("", "end", values=(chunk.tag, CHUNK_NAMES.get(chunk.marker, "Preserved block"), chunk.version, len(chunk.original), f"0x{chunk.offset:X}"))
        self.status()

    def save_dialog(self):
        if self.save is None:
            return
        def action():
            self.flush_forms()
            path = filedialog.asksaveasfilename(title="Save edited copy (choose a new filename)", initialdir=self.save.path.parent if self.save.path else None, initialfile=(self.save.path.stem if self.save.path else "save") + "_edited.sav", defaultextension=".sav", filetypes=[("Cultures saves", "*.sav")], confirmoverwrite=False, parent=self.root)
            if path:
                self.save.save_copy(Path(path))
                self.saved_bytes = self.save.encode()
                messagebox.showinfo("Copy saved", f"Saved edited copy:\n{path}\n\nThe source file is unchanged.", parent=self.root)
        self.run_action(action)

    def undo(self):
        def action():
            if self.save:
                self.flush_forms()
                self.close_dialogs()
                self.save.undo()
                self.refresh()
        self.run_action(action)

    def redo(self):
        def action():
            if self.save:
                self.flush_forms()
                self.close_dialogs()
                self.save.redo()
                self.refresh()
        self.run_action(action)

    def game_data(self):
        path = filedialog.askdirectory(title="Choose the game root containing DataX/libs", parent=self.root)
        if path:
            def action():
                self.flush_forms()
                self.catalog.load(Path(path))
                self.refresh()
            self.run_action(action)

    def export(self):
        if self.save is None:
            return
        def action():
            self.flush_forms()
            path = filedialog.asksaveasfilename(title="Export readable save information", defaultextension=".json", filetypes=[("JSON", "*.json")], parent=self.root)
            if not path:
                return
            report = self.save.summary()
            for tab in self.tabs:
                report["villagers" if tab.marker == HUMANS else "buildings"] = [dict(zip(tab.tree["columns"], row)) for row in tab.rows()]
            target = Path(path)
            if self.save.path is not None and target.resolve() == self.save.path.resolve():
                raise SaveError("The report cannot replace the source save.")
            with target.open("x", encoding="utf-8") as output:
                json.dump(report, output, ensure_ascii=False, indent=2)
        self.run_action(action)

    def export_map(self):
        if self.save is None:
            return
        def action():
            self.flush_forms()
            path = filedialog.asksaveasfilename(title="Choose a NEW folder for map.dat and map.ini", initialdir=self.save.path.parent if self.save.path else None, initialfile=(self.save.path.stem if self.save.path else "save") + "_map", filetypes=[("New map folder", "*")], confirmoverwrite=False, parent=self.root)
            if not path:
                return
            report = MapExport(self.save, self.catalog).write(Path(path))
            counts = report["counts"]
            messagebox.showinfo("Map exported", f"Created a new map folder:\n{path}\n\nmap.dat and map.ini\ntext/eng/strings.ini and text/ger/strings.ini\n\n{counts['villagers']} villagers, {counts['buildings']} buildings, {counts['animals']} animals and {counts['vehicles']} vehicles.\n\nThis creates a new starting map. Health, needs and other running state use startup defaults. Mission definitions/progress, scripted AI and fractional inventory are not exported. The source save is unchanged.", parent=self.root)
        self.run_action(action)

    def close(self):
        def action():
            if self.discard_ok():
                self.root.destroy()
        self.run_action(action)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("save", nargs="?", type=Path, help="Optional .sav to open")
    parser.add_argument("--inspect", type=Path, metavar="SAVE", help="Print a read-only JSON summary without opening the GUI")
    parser.add_argument("--game-dir", type=Path, help="Game root for optional type and stock definitions")
    parser.add_argument("--export-map", type=Path, metavar="NEW_FOLDER", help="Export the positional save as map.dat, map.ini and name strings without opening the GUI")
    arguments = parser.parse_args()
    if arguments.export_map:
        if arguments.save is None:
            parser.error("--export-map requires a positional save filename")
        try:
            game_dir = arguments.game_dir or next((parent for parent in arguments.save.resolve().parents if (parent / "DataX" / "libs").is_dir()), None)
            if game_dir is None:
                raise SaveError("Use --game-dir to select the matching game installation.")
            catalog = Catalog()
            catalog.load(game_dir)
            report = MapExport(Savegame.open(arguments.save), catalog).write(arguments.export_map)
            print(json.dumps(report, ensure_ascii=False, indent=2))
            return 0
        except ERRORS as error:
            print(f"Savegame Editor: {error}", file=sys.stderr)
            return 1
    if arguments.inspect:
        try:
            print(inspect_file(arguments.inspect))
            return 0
        except ERRORS as error:
            print(f"Savegame Editor: {error}", file=sys.stderr)
            return 1
    try:
        if sys.platform == "win32":
            import ctypes
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        root = tk.Tk()
        app = Editor(root)
        if arguments.game_dir:
            app.run_action(lambda: app.catalog.load(arguments.game_dir))
        if arguments.save:
            app.run_action(lambda: app.load(arguments.save))
        root.mainloop()
        return 0
    except tk.TclError as error:
        print(f"Unable to start Tkinter: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
