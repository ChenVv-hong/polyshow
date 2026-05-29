(function () {
    const app = document.getElementById("app");

    const normalLayers = [
        {
            id: "mixed",
            label: "mixed_primitives.ply (File)   4",
            objectLabel: "mixed_primitives.ply",
            type: "File layer",
            icon: "folder",
            visible: "mixed",
            primitives: 4,
            visibleCount: 4,
            hiddenCount: 0,
            polygons: 1,
            polylines: 1,
            points: 2,
            children: [
                { id: "point-1", label: "Point 1", icon: "radio_button_checked", visible: true },
                { id: "polyline-1", label: "Polyline 1", icon: "timeline", visible: true },
                { id: "polygon-1", label: "Polygon 1", icon: "pentagon", visible: true }
            ]
        },
        {
            id: "ipc-demo",
            label: "IPC Demo (IPC)   3",
            objectLabel: "IPC Demo",
            type: "IPC layer",
            icon: "settings_input_component",
            visible: true,
            primitives: 3,
            visibleCount: 2,
            hiddenCount: 1,
            polygons: 1,
            polylines: 0,
            points: 1,
            children: [
                { id: "stream-triangle", label: "stream_triangle", icon: "change_history", visible: true },
                { id: "stream-point", label: "Point 1", icon: "radio_button_checked", visible: false }
            ]
        }
    ];

    const searchLayers = [
        {
            id: "mixed",
            label: "mixed_primitives.ply (File)   4",
            objectLabel: "mixed_primitives.ply",
            type: "File layer",
            icon: "folder",
            visible: true,
            primitives: 4,
            visibleCount: 4,
            hiddenCount: 0,
            polygons: 1,
            polylines: 1,
            points: 2,
            children: [
                { id: "polyline-1", label: "Polyline 1", icon: "timeline", visible: true },
                { id: "polygon-1", label: "Polygon 1", icon: "pentagon", visible: true }
            ]
        },
        {
            id: "named-shapes",
            label: "named_shapes.ply (File)   3",
            objectLabel: "named_shapes.ply",
            type: "File layer",
            icon: "folder",
            visible: "mixed",
            primitives: 3,
            visibleCount: 2,
            hiddenCount: 1,
            polygons: 1,
            polylines: 1,
            points: 1,
            children: [
                { id: "named-polygon", label: "Named Polygon", icon: "pentagon", visible: true }
            ]
        }
    ];

    const menuModel = {
        File: [
            { icon: "add", label: "New Layer", shortcut: "Ctrl+N", dialog: "newLayer" },
            { icon: "folder_open", label: "Open", shortcut: "Ctrl+O", action: "open" },
            { icon: "ios_share", label: "Export Active Layer", shortcut: "Ctrl+E", dialog: "export" },
            { icon: "logout", label: "Exit", shortcut: "Alt+F4", action: "exit" }
        ],
        View: [
            { icon: "fit_screen", label: "Fit to View", shortcut: "F", action: "fit" },
            { icon: "zoom_in", label: "Zoom In", shortcut: "Ctrl++", action: "zoomIn" },
            { icon: "zoom_out", label: "Zoom Out", shortcut: "Ctrl+-", action: "zoomOut" },
            { icon: "center_focus_strong", label: "Reset View", shortcut: "Home", action: "reset" }
        ],
        Render: [
            { icon: "deployed_code", label: "Solid", action: "renderSolid" },
            { icon: "timeline", label: "Wireframe", action: "renderWireframe" },
            { icon: "scatter_plot", label: "Points", action: "renderPoints" }
        ],
        IPC: [
            { icon: "play_arrow", label: "Start IPC Listener", action: "startIpc", disabled: true },
            { icon: "stop", label: "Stop IPC Listener", action: "stopIpc" }
        ],
        Help: [
            { icon: "info", label: "About PolyShow", dialog: "about" }
        ]
    };

    const primitiveDetails = {
        "point-1": {
            label: "Point 1",
            kind: "Point",
            icon: "radio_button_checked",
            vertices: "1",
            bounds: "0.00 x 0.00",
            visible: "true",
            color: "#63A7FFFF",
            lineWidth: "1.00",
            pointSize: "4.00",
            coords: "260 138",
            selectionBox: { x: 252, y: 130, width: 26, height: 26 }
        },
        "polyline-1": {
            label: "Polyline 1",
            kind: "Polyline",
            icon: "timeline",
            vertices: "4",
            bounds: "170.00 x 72.00",
            visible: "true",
            color: "#F3A35CFF",
            lineWidth: "3.00",
            pointSize: "2.50",
            coords: "0 0\n50 40\n90 10\n140 60",
            selectionBox: { x: 358, y: 108, width: 188, height: 152 }
        },
        "polygon-1": {
            label: "Polygon 1",
            kind: "Polygon",
            icon: "pentagon",
            vertices: "5",
            bounds: "160.00 x 130.00",
            visible: "true",
            color: "#62D18B45",
            lineWidth: "2.00",
            pointSize: "2.50",
            coords: "25 38\n105 16\n148 78\n82 118\n18 92",
            selectionBox: { x: 358, y: 108, width: 188, height: 152 }
        },
        "stream-triangle": {
            label: "stream_triangle",
            kind: "Polygon",
            icon: "change_history",
            vertices: "3",
            bounds: "90.00 x 72.00",
            visible: "true",
            color: "#62D18B45",
            lineWidth: "2.00",
            pointSize: "2.50",
            coords: "20 0\n100 72\n0 72",
            selectionBox: { x: 358, y: 108, width: 188, height: 152 }
        },
        "stream-point": {
            label: "Point 1",
            kind: "Point",
            icon: "radio_button_checked",
            vertices: "1",
            bounds: "0.00 x 0.00",
            visible: "false",
            color: "#63A7FFFF",
            lineWidth: "1.00",
            pointSize: "4.00",
            coords: "590 312",
            selectionBox: { x: 582, y: 304, width: 28, height: 28 }
        },
        "named-polygon": {
            label: "Named Polygon",
            kind: "Polygon",
            icon: "pentagon",
            vertices: "5",
            bounds: "160.00 x 130.00",
            visible: "true",
            color: "#62D18B45",
            lineWidth: "2.00",
            pointSize: "2.50",
            coords: "25 38\n105 16\n148 78\n82 118\n18 92",
            selectionBox: { x: 358, y: 108, width: 188, height: 152 }
        }
    };

    const logSets = {
        primitive: [
            { level: "info", text: "[info] mixed_primitives.ply opened successfully" },
            { level: "info", text: "[info] Updated Polyline 1 in mixed_primitives.ply line width: 1.50 -> 3.00" }
        ],
        layer: [
            { level: "info", text: "[info] mixed_primitives.ply opened successfully" },
            { level: "warning", text: "[warning] Renamed imported layer square.ply to square.ply (2)" }
        ],
        search: [
            { level: "info", text: "[info] Search field filters layers and primitive names" },
            { level: "info", text: "[info] Appended IPC primitive stream_triangle to IPC Demo" }
        ],
        empty: []
    };

    const state = {
        selection: "primitive",
        selectedId: "polyline-1",
        searchOpen: false,
        query: "poly",
        activeMenu: "",
        dialog: "",
        renderMode: "Solid",
        drawingMode: "Browse",
        grid: true,
        logs: [...logSets.primitive]
    };

    function icon(name) {
        return `<span class="material-symbols-rounded" aria-hidden="true">${name}</span>`;
    }

    function logo() {
        return `
            <svg viewBox="0 0 64 64" aria-hidden="true">
                <path d="M32 7 L53 19 L53 45 L32 57 L11 45 L11 19 Z" fill="#20364c70" stroke="#63a7ff" stroke-width="5" stroke-linejoin="round"></path>
                <path d="M32 18 L45 26 L45 39 L32 47 L19 39 L19 26 Z" fill="#62d18b80" stroke="#62d18b" stroke-width="3" stroke-linejoin="round"></path>
                <path d="M32 18 L32 47 M19 39 L45 26" fill="none" stroke="#e7bf67" stroke-width="4" stroke-linecap="round"></path>
                <circle cx="32" cy="8" r="5" fill="#e7bf67" stroke="#202124" stroke-width="2"></circle>
                <circle cx="53" cy="45" r="5" fill="#e7bf67" stroke="#202124" stroke-width="2"></circle>
                <circle cx="11" cy="45" r="5" fill="#e7bf67" stroke="#202124" stroke-width="2"></circle>
                <circle cx="32" cy="32" r="5" fill="#63a7ff" stroke="#202124" stroke-width="2"></circle>
            </svg>
        `;
    }

    function treeLayers() {
        if (state.selection === "empty") {
            return [];
        }
        return state.searchOpen ? searchLayers : normalLayers;
    }

    function currentPrimitive() {
        return primitiveDetails[state.selectedId] || primitiveDetails["polyline-1"];
    }

    function currentLayer() {
        const layers = [...normalLayers, ...searchLayers];
        return layers.find((layer) => layer.id === state.selectedId) || normalLayers[0];
    }

    function render() {
        app.className = [
            "polyshow-shell",
            state.selection === "empty" ? "empty" : "",
            state.searchOpen ? "search-open" : ""
        ].filter(Boolean).join(" ");
        app.dataset.selection = state.selection;

        app.innerHTML = `
            ${renderTopbar()}
            <section class="workspace">
                <section class="main-split">
                    ${renderOutliner()}
                    ${renderViewport()}
                    ${renderInspector()}
                </section>
                ${renderLog()}
            </section>
            ${renderStatusbar()}
            ${state.activeMenu ? renderMenuPopover(state.activeMenu) : ""}
            ${state.dialog ? renderDialog(state.dialog) : ""}
        `;

        bindEvents();
    }

    function renderTopbar() {
        return `
            <header class="topbar">
                <div class="brand">${logo()}<span>PolyShow</span></div>
                <nav class="menu-strip" aria-label="Application menu">
                    ${Object.keys(menuModel).map((name) => `
                        <button class="menu-button ${state.activeMenu === name ? "active" : ""}" type="button" data-menu="${name}">${name}</button>
                    `).join("")}
                </nav>
                <div class="top-spacer"></div>
                <div class="tabs" aria-label="Workspace tabs">
                    <button class="tab active" type="button">Modeling</button>
                    <button class="tab" type="button">Inspect</button>
                    <button class="tab" type="button">IPC</button>
                </div>
            </header>
        `;
    }

    function renderOutliner() {
        return `
            <aside class="panel outliner">
                <header class="panel-header outliner-header">
                    ${icon("account_tree")}
                    <span>Outliner</span>
                    <div class="outliner-tools">
                        <button class="icon-button" type="button" title="New Layer" data-dialog="newLayer">${icon("add")}</button>
                        <button class="icon-button" type="button" title="Export Layer" data-dialog="export">${icon("ios_share")}</button>
                        <button class="icon-button ${state.searchOpen ? "active" : ""}" type="button" title="Search" data-action="toggleSearch">${icon("search")}</button>
                    </div>
                </header>
                ${state.searchOpen && state.selection !== "empty" ? renderSearchField() : ""}
                <div class="tree">
                    ${state.selection === "empty" ? renderEmptyOutliner() : treeLayers().map(renderLayer).join("")}
                </div>
                <div class="summary"><span>${outlinerFooter()}</span></div>
            </aside>
        `;
    }

    function renderSearchField() {
        return `
            <div class="search-bar">
                <label class="search-field">
                    ${icon("search")}
                    <input class="search-input" type="text" value="${escapeHtml(state.query)}" aria-label="Search query">
                </label>
            </div>
        `;
    }

    function renderEmptyOutliner() {
        return `
            <div class="outliner-empty">
                ${icon("folder_open")}
                <strong>No layers</strong>
                <span>Open .ply or create a layer</span>
            </div>
        `;
    }

    function outlinerFooter() {
        if (state.selection === "empty") {
            return "0 layers / 0 visible primitives";
        }
        if (state.searchOpen) {
            return `Query: ${state.query || "poly"} / 5 matches`;
        }
        return "2 layers / 6 visible primitives";
    }

    function renderLayer(layer) {
        const selected = state.selection === "layer" && state.selectedId === layer.id;
        return `
            ${renderTreeRow({
                id: layer.id,
                select: "layer",
                label: layer.label,
                iconName: layer.icon,
                visible: layer.visible,
                selected,
                indent: false
            })}
            ${layer.children.map((child) => renderPrimitive(child)).join("")}
        `;
    }

    function renderPrimitive(item) {
        return renderTreeRow({
            id: item.id,
            select: "primitive",
            label: item.label,
            iconName: item.icon,
            visible: item.visible,
            selected: state.selection === "primitive" && state.selectedId === item.id,
            indent: true
        });
    }

    function renderTreeRow(item) {
        const muted = item.visible === false;
        return `
            <div class="tree-row ${item.indent ? "child" : ""} ${item.selected ? "selected" : ""} ${muted ? "hidden-row" : ""}"
                data-select="${item.select}" data-id="${item.id}">
                <span class="row-indent"></span>
                ${icon(item.iconName)}
                ${renderVisibility(item.visible)}
                <span class="label">${escapeHtml(item.label)}</span>
            </div>
        `;
    }

    function renderVisibility(visible) {
        if (visible === "mixed") {
            return '<span class="check mixed"><span></span></span>';
        }
        return `<span class="check ${visible ? "on" : ""}">${visible ? icon("check") : ""}</span>`;
    }

    function renderViewport() {
        return `
            <main class="viewport-panel">
                <div class="viewport-toolbar">
                    ${toolButton("near_me", "Browse", state.drawingMode === "Browse", "modeBrowse")}
                    ${toolButton("radio_button_checked", "Point", state.drawingMode === "Point", "modePoint")}
                    ${toolButton("timeline", "Line", state.drawingMode === "Line", "modeLine")}
                    ${toolButton("pentagon", "Poly", state.drawingMode === "Poly", "modePoly")}
                    ${toolButton("done", "Done", false, "done", state.drawingMode === "Browse")}
                    ${toolButton("close", "", false, "cancel", state.drawingMode === "Browse")}
                    <div class="toolbar-spacer"></div>
                    <button class="icon-button" type="button" title="Fit View" data-action="fit">${icon("fit_screen")}</button>
                    <button class="icon-button" type="button" title="Zoom Out" data-action="zoomOut">${icon("zoom_out")}</button>
                    <button class="icon-button" type="button" title="Zoom In" data-action="zoomIn">${icon("zoom_in")}</button>
                    <button class="icon-button" type="button" title="Reset View" data-action="reset">${icon("center_focus_strong")}</button>
                    <button class="icon-button ${state.grid ? "active" : ""}" type="button" title="Toggle Grid" data-action="toggleGrid">${icon("grid_on")}</button>
                    <button class="render-mode-button" type="button" title="Render Mode" data-action="renderCycle">
                        ${icon("deployed_code")}<span>${escapeHtml(state.renderMode)}</span>${icon("expand_more")}
                    </button>
                </div>
                <section class="canvas-wrap">
                    ${state.grid ? '<div class="canvas-grid"></div>' : ""}
                    ${renderGeometry()}
                    ${state.selection !== "empty" ? renderViewportOverlays() : ""}
                </section>
            </main>
        `;
    }

    function toolButton(iconName, label, active, action, disabled) {
        const title = label || (action === "cancel" ? "Cancel" : iconName);
        return `
            <button class="tool-button ${active ? "active" : ""} ${disabled ? "disabled" : ""}" type="button" title="${escapeHtml(title)}" data-action="${action}">
                ${icon(iconName)}${label ? `<span>${label}</span>` : ""}
            </button>
        `;
    }

    function renderGeometry() {
        const hidden = state.selection === "empty";
        if (hidden) {
            return '<svg class="canvas geometry-overlay" viewBox="0 0 640 480" preserveAspectRatio="xMidYMid meet" aria-label="PolyShow empty viewport"></svg>';
        }

        const solid = state.renderMode === "Solid";
        const pointsOnly = state.renderMode === "Points";
        const polygonFill = solid && !pointsOnly ? "#62d18b45" : "transparent";
        const polylineOpacity = pointsOnly ? "0.18" : "1";
        const polygonOpacity = pointsOnly ? "0.18" : "1";
        const primitive = currentPrimitive();
        const primitiveBox = primitive.selectionBox;
        const layerBox = { x: 130, y: 100, width: 482, height: 248 };
        const selectionBox = state.selection === "layer" ? layerBox : primitiveBox;
        const selectionVisible = !hidden && (state.selection === "primitive" || state.selection === "layer");

        return `
            <svg class="canvas geometry-overlay" viewBox="0 0 640 480" preserveAspectRatio="xMidYMid meet" aria-label="PolyShow viewport geometry">
                <path data-select="primitive" data-id="polygon-1" d="M395 158 L475 136 L518 198 L452 238 L388 212 Z" fill="${polygonFill}" stroke="#62d18b" stroke-width="${pointsOnly ? 1 : 2}" stroke-linejoin="round" opacity="${polygonOpacity}"></path>
                <path data-select="primitive" data-id="polyline-1" d="M154 302 L208 252 L271 271 L322 241" fill="none" stroke="#f3a35c" stroke-width="${pointsOnly ? 1 : 3}" stroke-linecap="round" stroke-linejoin="round" opacity="${polylineOpacity}"></path>
                <circle data-select="primitive" data-id="point-1" cx="265" cy="143" r="5" fill="#63a7ff" stroke="#63a7ff" stroke-width="1"></circle>
                <circle data-select="primitive" data-id="stream-point" cx="596" cy="318" r="6" fill="#63a7ff" stroke="#63a7ff" stroke-width="1"></circle>
                ${selectionVisible ? `<rect x="${selectionBox.x}" y="${selectionBox.y}" width="${selectionBox.width}" height="${selectionBox.height}" fill="none" stroke="#63a7ff" stroke-width="1.5"></rect>` : ""}
                ${renderVertexPoints(pointsOnly)}
            </svg>
        `;
    }

    function renderVertexPoints(pointsOnly) {
        if (!pointsOnly || state.selection === "empty") {
            return "";
        }

        const points = [
            [395, 158], [475, 136], [518, 198], [452, 238], [388, 212],
            [154, 302], [208, 252], [271, 271], [322, 241]
        ];

        return points.map(([x, y]) => `<circle cx="${x}" cy="${y}" r="4" fill="#e7bf67" stroke="#1d2228" stroke-width="1.5"></circle>`).join("");
    }

    function renderViewportOverlays() {
        return `
            <div class="hint-overlay">
                <div class="hint-row">${icon("mouse")}<span>Zoom</span></div>
                <div class="hint-row">${icon("pan_tool")}<span>Pan</span></div>
                <div class="hint-row">${icon("ads_click")}<span>Select</span></div>
            </div>
            <div class="scale-overlay"><div>100</div><div class="scale-mark"></div></div>
        `;
    }

    function renderInspector() {
        if (state.selection === "empty") {
            return '<aside class="panel inspector" aria-hidden="true"></aside>';
        }

        return `
            <aside class="panel inspector">
                <header class="panel-header properties-header">
                    ${icon("tune")}
                    <span>Properties</span>
                    <span class="mode-chip">${state.selection === "layer" ? "Layer" : "Primitive"}</span>
                </header>
                ${state.selection === "layer" ? renderLayerInspector() : renderPrimitiveInspector()}
            </aside>
        `;
    }

    function renderPrimitiveInspector() {
        const primitive = currentPrimitive();
        return `
            ${objectRow(primitive.icon, primitive.label)}
            <div class="inspector-body">
                ${section("Geometry", `
                    ${field(primitive.icon, "Type", primitive.kind)}
                    ${field("hub", "Vertices", primitive.vertices)}
                    ${field("select_all", "Bounds", primitive.bounds)}
                    ${field("visibility", "Visible", primitive.visible)}
                `)}
                ${section("Style", `
                    ${control("Stroke Color", `<div class="color-field"><span class="swatch" style="background:#f3a35c"></span>${icon("palette")}<code>${escapeHtml(primitive.color)}</code></div>`)}
                    ${control("Line Width", `<div class="text-field">${icon("line_weight")}<code>${escapeHtml(primitive.lineWidth)}</code></div>`)}
                    ${control("Point Size", `<div class="text-field">${icon("radio_button_checked")}<code>${escapeHtml(primitive.pointSize)}</code></div>`)}
                `)}
                ${section("Coordinates", `
                    <textarea class="coord-field" spellcheck="false">${escapeHtml(primitive.coords)}</textarea>
                    <div class="inspector-hint">Invalid coordinates hide preview.</div>
                `)}
            </div>
        `;
    }

    function renderLayerInspector() {
        const layer = currentLayer();
        return `
            ${objectRow(layer.icon, layer.objectLabel)}
            <div class="inspector-body">
                ${section("Summary", `
                    ${field("folder", "Type", layer.type)}
                    ${field("category", "Primitives", String(layer.primitives))}
                    ${field("visibility", "Visible", String(layer.visibleCount))}
                    ${field("visibility_off", "Hidden", String(layer.hiddenCount))}
                    ${field("pentagon", "Polygons", String(layer.polygons))}
                    ${field("timeline", "Polylines", String(layer.polylines))}
                    ${field("radio_button_checked", "Points", String(layer.points))}
                    <div class="inspector-hint bordered">Select a primitive to edit style and coordinates.</div>
                `)}
            </div>
        `;
    }

    function objectRow(iconName, label) {
        return `
            <div class="properties-object">
                ${icon(iconName)}
                <span>${escapeHtml(label)}</span>
            </div>
        `;
    }

    function section(title, body) {
        return `
            <section class="section">
                <div class="section-title">${icon("expand_more")}<span>${title}</span></div>
                <div class="section-content">${body}</div>
            </section>
        `;
    }

    function field(iconName, key, value) {
        return `
            <div class="field-row">
                ${icon(iconName)}
                <span>${escapeHtml(key)}</span>
                <code>${escapeHtml(value)}</code>
            </div>
        `;
    }

    function control(label, body) {
        return `
            <div class="control-group">
                <div class="control-label">${escapeHtml(label)}</div>
                ${body}
            </div>
        `;
    }

    function renderLog() {
        return `
            <section class="log-panel">
                <header class="log-head">
                    ${icon("terminal")}
                    <span>Log</span>
                    <span class="log-spacer"></span>
                    ${icon("keyboard_arrow_down")}
                </header>
                <div class="log-body">
                    ${state.logs.map((entry) => `
                        <div class="log-row ${entry.level}">
                            ${icon(entry.level === "warning" ? "warning" : entry.level === "error" ? "error" : "info")}
                            <span>${escapeHtml(entry.text)}</span>
                        </div>
                    `).join("")}
                </div>
            </section>
        `;
    }

    function renderStatusbar() {
        const empty = state.selection === "empty";
        const message = empty ? "Ready" : state.searchOpen || state.selection === "layer" ? "Ready" : "Primitive updated";
        const counts = empty ? "Points: 0  Polylines: 0  Polygons: 0" : "Points: 2  Polylines: 1  Polygons: 1";

        return `
            <footer class="statusbar">
                ${icon("info")}
                <span>${message}</span>
                <div class="status-spacer"></div>
                <code>${counts}</code>
                <code>X: 0.00&nbsp;&nbsp;Y: 0.00</code>
            </footer>
        `;
    }

    function renderMenuPopover(menuName) {
        const left = { File: 104, View: 150, Render: 200, IPC: 270, Help: 310 }[menuName] || 104;
        const items = menuModel[menuName] || [];
        return `
            <div class="menu-popover" style="left:${left}px; top:38px" role="menu">
                ${items.map((item) => `
                    <button class="menu-item ${item.disabled ? "disabled" : ""}" type="button"
                        data-action="${item.action || ""}" data-dialog="${item.dialog || ""}" role="menuitem">
                        ${icon(item.icon)}<span>${item.label}</span><span class="shortcut">${item.shortcut || ""}</span>
                    </button>
                `).join("")}
            </div>
        `;
    }

    function renderDialog(name) {
        const configs = {
            newLayer: {
                icon: "add",
                title: "New Layer",
                body: `
                    <div class="dialog-grid">
                        <label for="layer-name">Name</label><input id="layer-name" value="mixed_primitives">
                        <label for="layer-type">Type</label><select id="layer-type"><option>File layer</option><option>IPC layer</option><option>Internal layer</option></select>
                    </div>
                    <p class="dialog-error">Layer name must be unique within the document.</p>
                `
            },
            about: {
                icon: "info",
                title: "About PolyShow",
                wide: true,
                body: `<p>PolyShow MVP<br><br>Features:<br>1. Create, open, and export layers<br>2. Display points, polylines, and polygons<br>3. Draw primitives directly in the workspace<br>4. Toggle layer and primitive visibility<br>5. Search and inspect opened geometry<br>6. Switch render mode (Solid/Wireframe/Points)</p>`
            },
            export: {
                icon: "ios_share",
                title: "Export Layer",
                body: "<p>Select or create a layer first.</p>"
            },
            openFailed: {
                icon: "error",
                title: "Open Failed",
                body: "<p>No files were opened.<br><br>invalid_bad_number.ply: line 4 failed to parse a number.</p>"
            }
        };
        const config = configs[name] || configs.about;

        return `
            <div class="modal-layer open">
                <section class="dialog ${config.wide ? "wide" : ""}" role="dialog" aria-modal="true" aria-label="${config.title}">
                    <div class="dialog-title">${icon(config.icon)}<span>${config.title}</span></div>
                    <div class="dialog-body">${config.body}</div>
                    <div class="dialog-actions">
                        <button class="small-button ${name === "newLayer" ? "primary-action disabled" : "primary-action"}" type="button" data-action="confirmDialog">OK</button>
                        ${name === "newLayer" ? '<button class="small-button" type="button" data-action="closeDialog">Cancel</button>' : ""}
                    </div>
                </section>
            </div>
        `;
    }

    function bindEvents() {
        app.querySelectorAll("[data-menu]").forEach((button) => {
            button.addEventListener("click", () => {
                const menu = button.dataset.menu;
                state.activeMenu = state.activeMenu === menu ? "" : menu;
                state.dialog = "";
                render();
            });
        });

        app.querySelectorAll("[data-select]").forEach((item) => {
            item.addEventListener("click", (event) => {
                event.stopPropagation();
                state.selection = item.dataset.select;
                state.selectedId = item.dataset.id;
                state.activeMenu = "";
                if (state.selection === "layer") {
                    state.searchOpen = false;
                    state.query = "poly";
                    state.logs = [...logSets.layer];
                } else {
                    state.logs = state.searchOpen ? [...logSets.search] : [...logSets.primitive];
                }
                render();
            });
        });

        app.querySelectorAll("[data-action]").forEach((button) => {
            button.addEventListener("click", (event) => {
                event.stopPropagation();
                handleAction(button.dataset.action);
            });
        });

        app.querySelectorAll("[data-dialog]").forEach((button) => {
            const dialog = button.dataset.dialog;
            if (!dialog) {
                return;
            }
            button.addEventListener("click", (event) => {
                event.stopPropagation();
                state.dialog = dialog;
                state.activeMenu = "";
                render();
            });
        });

        const searchInput = app.querySelector(".search-input");
        if (searchInput) {
            searchInput.addEventListener("input", (event) => {
                state.query = event.target.value;
                const summary = app.querySelector(".summary span");
                if (summary) {
                    summary.textContent = outlinerFooter();
                }
            });
        }

        const canvasWrap = app.querySelector(".canvas-wrap");
        if (canvasWrap) {
            canvasWrap.addEventListener("click", () => {
                if (state.selection !== "empty") {
                    state.selection = "empty";
                    state.selectedId = "";
                    state.searchOpen = false;
                    state.activeMenu = "";
                    state.logs = [...logSets.empty];
                    render();
                }
            });
        }

        app.querySelector(".modal-layer")?.addEventListener("click", (event) => {
            if (event.target.classList.contains("modal-layer")) {
                state.dialog = "";
                render();
            }
        });
    }

    function handleAction(action) {
        if (!action) {
            return;
        }

        const actions = {
            toggleSearch() {
                state.searchOpen = !state.searchOpen;
                state.selection = "primitive";
                state.selectedId = "polyline-1";
                state.query = "poly";
                state.logs = state.searchOpen ? [...logSets.search] : [...logSets.primitive];
            },
            modeBrowse() {
                state.drawingMode = "Browse";
                addLog("info", "[info] Browse mode active");
            },
            modePoint() {
                state.drawingMode = "Point";
                addLog("info", "[info] Point drawing mode active");
            },
            modeLine() {
                state.drawingMode = "Line";
                addLog("info", "[info] Polyline drawing: 3 vertex/vertices");
            },
            modePoly() {
                state.drawingMode = "Poly";
                addLog("info", "[info] Polygon drawing mode active");
            },
            done() {
                state.drawingMode = "Browse";
                addLog("info", "[info] Drawing committed");
            },
            cancel() {
                state.drawingMode = "Browse";
                addLog("warning", "[warning] Drawing cancelled");
            },
            fit() {
                addLog("info", "[info] Fit to view applied");
            },
            zoomIn() {
                addLog("info", "[info] Zoom in");
            },
            zoomOut() {
                addLog("info", "[info] Zoom out");
            },
            reset() {
                addLog("info", "[info] View reset");
            },
            toggleGrid() {
                state.grid = !state.grid;
            },
            renderCycle() {
                const modes = ["Solid", "Wireframe", "Points"];
                state.renderMode = modes[(modes.indexOf(state.renderMode) + 1) % modes.length];
                addLog("info", `[info] Render mode changed: ${state.renderMode}`);
            },
            renderSolid() {
                state.renderMode = "Solid";
            },
            renderWireframe() {
                state.renderMode = "Wireframe";
            },
            renderPoints() {
                state.renderMode = "Points";
            },
            open() {
                state.dialog = "openFailed";
                addLog("error", "[error] invalid_bad_number.ply: line 4 failed to parse a number");
            },
            exit() {
                addLog("warning", "[warning] Exit is disabled in the web prototype");
            },
            startIpc() {
                addLog("info", "[info] IPC listener started on \\\\.\\pipe\\polyshow-ipc");
            },
            stopIpc() {
                addLog("info", "[info] IPC listener stopped");
            },
            closeDialog() {
                state.dialog = "";
            },
            confirmDialog() {
                state.dialog = "";
            }
        };

        actions[action]?.();
        if (action.startsWith("render")) {
            state.activeMenu = "";
        } else if (!["closeDialog", "confirmDialog"].includes(action)) {
            state.activeMenu = "";
        }
        render();
    }

    function addLog(level, text) {
        if (state.selection === "empty") {
            state.selection = "primitive";
            state.selectedId = "polyline-1";
        }
        state.logs = [...state.logs.slice(-3), { level, text }];
    }

    function escapeHtml(value) {
        return String(value)
            .replaceAll("&", "&amp;")
            .replaceAll("<", "&lt;")
            .replaceAll(">", "&gt;")
            .replaceAll('"', "&quot;");
    }

    document.addEventListener("keydown", (event) => {
        if (event.key === "Escape") {
            state.activeMenu = "";
            state.dialog = "";
            render();
        }
        if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "f") {
            event.preventDefault();
            state.searchOpen = true;
            state.selection = "primitive";
            state.selectedId = "polyline-1";
            state.query = "poly";
            state.logs = [...logSets.search];
            render();
        }
    });

    render();
}());
