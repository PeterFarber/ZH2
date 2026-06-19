#!/usr/bin/env python3
"""Derive readable Graphify community names without requiring an LLM backend."""

from __future__ import annotations

import argparse
import html
import json
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


OWNER_NAMES = {
    "libs/core": "Core",
    "libs/ecs": "ECS",
    "libs/assets": "Assets",
    "libs/renderer": "Renderer",
    "libs/physics": "Physics",
    "libs/gameplay": "Gameplay",
    "libs/editor": "Editor",
    "apps/game_client": "Game Client",
    "apps/map_editor": "Map Editor",
    "apps/dedicated_server": "Dedicated Server",
    "apps/asset_tool": "Asset Tool",
    "apps/shader_tool": "Shader Tool",
    "apps/core_tests": "Core Tests",
    "apps/ecs_tests": "ECS Tests",
    "apps/asset_tests": "Asset Tests",
    "apps/renderer_tests": "Renderer Tests",
    "apps/editor_tests": "Editor Tests",
    "apps/physics_tests": "Physics Tests",
    "apps/gameplay_tests": "Gameplay Tests",
}

ACRONYMS = {
    "aabb": "AABB",
    "api": "API",
    "cli": "CLI",
    "cmake": "CMake",
    "cpu": "CPU",
    "ecs": "ECS",
    "fxaa": "FXAA",
    "gltf": "glTF",
    "gpu": "GPU",
    "gui": "GUI",
    "id": "ID",
    "ids": "IDs",
    "imgui": "ImGui",
    "json": "JSON",
    "pbr": "PBR",
    "png": "PNG",
    "raii": "RAII",
    "sdl": "SDL",
    "ssao": "SSAO",
    "spv": "SPIR-V",
    "tga": "TGA",
    "toml": "TOML",
    "ui": "UI",
    "uuid": "UUID",
    "vk": "Vulkan",
    "vulkan": "Vulkan",
    "yaml": "YAML",
}

GENERIC_PARTS = {
    "apps",
    "build",
    "c",
    "cc",
    "cpp",
    "cxx",
    "data",
    "docs",
    "h",
    "hpp",
    "hockey",
    "include",
    "lib",
    "libs",
    "md",
    "src",
    "test",
    "tests",
}

GENERIC_LABELS = {
    "apply",
    "bool",
    "char",
    "count",
    "data",
    "double",
    "entity",
    "execute",
    "failed",
    "float",
    "id",
    "index",
    "init",
    "int",
    "main",
    "name",
    "path",
    "passed",
    "result",
    "run",
    "scene",
    "shutdown",
    "size",
    "size_t",
    "stats",
    "status",
    "std",
    "string",
    "type",
    "uint32_t",
    "undo",
    "uuid",
    "value",
    "vec2",
    "vec3",
    "vec4",
    "vector",
    "void",
}


def split_words(value: str) -> list[str]:
    value = re.sub(r"\.(cpp|cxx|cc|hpp|h)$", "", value, flags=re.IGNORECASE)
    value = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", value)
    value = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1 \2", value)
    value = re.sub(r"[^A-Za-z0-9]+", " ", value)
    return [part for part in value.strip().split() if part]


def humanize(value: str) -> str:
    words = split_words(value)
    rendered: list[str] = []
    for word in words:
        lowered = word.lower()
        rendered.append(ACRONYMS.get(lowered, lowered.capitalize()))
    return " ".join(rendered)


def clean_label(label: str) -> str | None:
    label = label.strip()
    label = label.removeprefix(".")
    label = re.sub(r"\(\)$", "", label)
    label = re.sub(r"^m_", "", label)
    label = Path(label).stem if re.search(r"\.(cpp|cxx|cc|hpp|h)$", label, re.IGNORECASE) else label
    words = split_words(label)
    if not words:
        return None
    lowered = " ".join(word.lower() for word in words)
    if lowered in GENERIC_LABELS:
        return None
    if len(words) == 1 and len(words[0]) < 3 and words[0].lower() not in ACRONYMS:
        return None
    return humanize(label)


def normalized_source_path(node: dict[str, Any]) -> str | None:
    source_file = node.get("source_file")
    if not isinstance(source_file, str) or not source_file:
        return None
    return source_file.replace("\\", "/").strip("/")


def owner_for_path(path: str) -> tuple[str, list[str]]:
    parts = path.split("/")
    if len(parts) >= 2:
        key = f"{parts[0]}/{parts[1]}"
        if key in OWNER_NAMES:
            return OWNER_NAMES[key], parts[2:]
    if parts:
        first = parts[0].lower()
        if first == "data" and len(parts) >= 2:
            return f"Data: {humanize(parts[1])}", parts[2:]
        if first == "scripts" and len(parts) >= 2:
            return f"Scripts: {humanize(parts[1])}", parts[2:]
        if first == "docs":
            return "Docs", parts[1:]
        if first in {"cmakelists.txt", "cmakepresets.json", "vcpkg.json"}:
            return "Build", parts[1:]
    return "Project", parts[1:]


def path_terms(path: str, remainder: list[str]) -> list[str]:
    terms: list[str] = []
    filtered = [part for part in remainder if part.lower() not in GENERIC_PARTS]

    if path:
        stem = Path(path).stem
        if stem and stem.lower() not in GENERIC_PARTS:
            terms.append(humanize(stem))

    for part in filtered:
        stem = Path(part).stem
        lowered = stem.lower()
        if lowered and lowered not in GENERIC_PARTS:
            terms.append(humanize(stem))

    return terms


def compact_topics(topics: Counter[str], owner: str, limit: int = 2) -> list[str]:
    owner_words = {word.lower() for word in split_words(owner)}
    selected: list[str] = []
    seen: set[str] = set()

    for topic, _count in topics.most_common():
        topic = topic.strip()
        if not topic:
            continue
        topic_key = topic.lower()
        if topic_key in seen:
            continue
        topic_words = {word.lower() for word in split_words(topic)}
        if topic_words and topic_words.issubset(owner_words):
            continue
        if topic_key in GENERIC_LABELS:
            continue
        selected.append(topic)
        seen.add(topic_key)
        if len(selected) >= limit:
            break

    return selected


def limit_name(value: str, max_len: int = 74) -> str:
    value = re.sub(r"\s+", " ", value).strip(" -:/")
    value = value.replace('"', "").replace("`", "")
    if len(value) <= max_len:
        return value
    shortened = value[: max_len - 3].rsplit(" ", 1)[0].strip(" -:/")
    return f"{shortened}..."


def community_name(nodes: list[dict[str, Any]]) -> str:
    owners: Counter[str] = Counter()
    topics: Counter[str] = Counter()

    for node in nodes:
        source_path = normalized_source_path(node)
        if source_path:
            owner, remainder = owner_for_path(source_path)
            owners[owner] += 1
            for term in path_terms(source_path, remainder):
                topics[term] += 4

        label = node.get("label")
        if isinstance(label, str):
            cleaned = clean_label(label)
            if cleaned:
                topics[cleaned] += 2

    owner = owners.most_common(1)[0][0] if owners else "Project"
    selected_topics = compact_topics(topics, owner)

    if selected_topics:
        topic_text = " / ".join(selected_topics)
        if owner == "Project":
            return limit_name(topic_text)
        return limit_name(f"{owner}: {topic_text}")

    return owner


def build_names(graph: dict[str, Any]) -> dict[int, str]:
    communities: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for node in graph.get("nodes", []):
        community = node.get("community")
        if isinstance(community, int):
            communities[community].append(node)

    return {community: community_name(nodes) for community, nodes in sorted(communities.items())}


def update_graph(graph_path: Path, names: dict[int, str]) -> None:
    graph = json.loads(graph_path.read_text(encoding="utf-8"))
    for node in graph.get("nodes", []):
        community = node.get("community")
        if isinstance(community, int) and community in names:
            node["community_name"] = names[community]
    graph_path.write_text(json.dumps(graph, indent=2) + "\n", encoding="utf-8")


def update_label_cache(output_dir: Path, names: dict[int, str]) -> None:
    cache_path = output_dir / ".graphify_labels.json"
    cache = {str(community): name for community, name in sorted(names.items())}
    cache_path.write_text(json.dumps(cache, indent=2) + "\n", encoding="utf-8")


def update_name_index(output_dir: Path, names: dict[int, str]) -> None:
    index_path = output_dir / "community-names.json"
    index = [
        {"community": community, "name": name}
        for community, name in sorted(names.items())
    ]
    index_path.write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")


def patch_report(report_path: Path, names: dict[int, str]) -> bool:
    if not report_path.exists():
        return False

    heading_re = re.compile(r'^### Community (\d+) - ".*"$')
    hub_re = re.compile(r"^- \[\[_COMMUNITY_Community (\d+)\|.*\]\]$")
    community_ref_re = re.compile(r"(?<!_COMMUNITY_)Community (\d+)(?: \([^`\n)]*\))?")

    patched_lines: list[str] = []
    for line in report_path.read_text(encoding="utf-8").splitlines():
        heading = heading_re.match(line)
        if heading:
            community = int(heading.group(1))
            patched_lines.append(f'### Community {community} - "{names.get(community, line)}"')
            continue

        hub = hub_re.match(line)
        if hub:
            community = int(hub.group(1))
            label = names.get(community, f"Community {community}")
            patched_lines.append(f"- [[_COMMUNITY_Community {community}|Community {community} - {label}]]")
            continue

        def replace_ref(match: re.Match[str]) -> str:
            community = int(match.group(1))
            label = names.get(community)
            if not label:
                return match.group(0)
            return f"Community {community} ({label})"

        patched_lines.append(community_ref_re.sub(replace_ref, line))

    report_path.write_text("\n".join(patched_lines) + "\n", encoding="utf-8")
    return True


def patch_callflow(callflow_path: Path, names: dict[int, str]) -> bool:
    if not callflow_path.exists():
        return False

    text = callflow_path.read_text(encoding="utf-8")

    for community, name in sorted(names.items(), key=lambda item: len(str(item[0])), reverse=True):
        escaped = html.escape(name, quote=True)
        json_value = json.dumps(name)[1:-1]
        text = text.replace(
            f'"community_name": "Community {community}"',
            f'"community_name": "{json_value}"',
        )
        text = re.sub(
            rf">(Community {community})(?: - [^<]+)?<",
            f">Community {community} - {escaped}<",
            text,
        )
        text = re.sub(
            rf'(<h2 id="community-{community}">\d+\. )Community {community}(?: - [^<]+)?</h2>',
            rf"\1Community {community} - {escaped}</h2>",
            text,
        )
        text = re.sub(
            rf"(<!-- ====== \d+\. )Community {community}(?: - [^=]+)?( ====== -->)",
            rf"\1Community {community} - {name}\2",
            text,
        )
        text = re.sub(
            rf'"(Community {community})(?: - [^"<]+)?<br/>',
            f'"Community {community} - {escaped}<br/>',
            text,
        )
        text = re.sub(
            rf"(Community {community})(?: \([^)\n]+\))? groups implementation",
            f"Community {community} ({escaped}) groups implementation",
            text,
        )
        text = re.sub(
            rf"(%% Section: Community {community})(?: - [^\n]+)?",
            f"%% Section: Community {community} - {name}",
            text,
        )

    callflow_path.write_text(text, encoding="utf-8")
    return True


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--graph",
        default="graphify-out/graph.json",
        type=Path,
        help="Path to Graphify graph.json.",
    )
    parser.add_argument(
        "--output-dir",
        default=Path("graphify-out"),
        type=Path,
        help="Graphify output directory containing GRAPH_REPORT.md and HTML.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    graph_path = args.graph
    output_dir = args.output_dir
    if not graph_path.exists():
        raise SystemExit(f"Missing graph file: {graph_path}")

    graph = json.loads(graph_path.read_text(encoding="utf-8"))
    names = build_names(graph)
    if not names:
        raise SystemExit("No communities found in graph.")

    update_graph(graph_path, names)
    update_label_cache(output_dir, names)
    update_name_index(output_dir, names)
    report_updated = patch_report(output_dir / "GRAPH_REPORT.md", names)
    callflow_updated = patch_callflow(output_dir / "ZH2-callflow.html", names)
    graph_html_updated = patch_callflow(output_dir / "graph.html", names)

    print(f"Named {len(names)} Graphify communities.")
    print(f"Updated {graph_path}")
    print(f"Updated {output_dir / '.graphify_labels.json'}")
    print(f"Updated {output_dir / 'community-names.json'}")
    if report_updated:
        print(f"Updated {output_dir / 'GRAPH_REPORT.md'}")
    if callflow_updated:
        print(f"Updated {output_dir / 'ZH2-callflow.html'}")
    if graph_html_updated:
        print(f"Updated {output_dir / 'graph.html'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
