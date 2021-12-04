from rich.console import Console
from rich.panel import Panel

console = Console()
description = """
[blue]text[/blue] text
[blue]text[/blue] text
"""

console.print(
    Panel(
        description.strip(),
        expand=True,
        title="PSHMode 2.0"
    )
)
