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
<<<<<<< Updated upstream
        title="psh-mode 2.0"
=======
        title="PSHMode 2.0"
>>>>>>> Stashed changes
    )
)
