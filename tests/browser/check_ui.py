"""Exercise the embedded page against scripts/preview.py's real native API."""
from pathlib import Path
import os
from playwright.sync_api import sync_playwright, expect

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "test-results"
OUTPUT.mkdir(exist_ok=True)
URL = os.environ.get("PREVIEW_URL", "http://127.0.0.1:8080")

with sync_playwright() as playwright:
    browser = playwright.chromium.launch(channel="chromium", args=["--disable-dev-shm-usage"])
    page = browser.new_page(viewport={"width": 1440, "height": 900})
    errors = []
    external_requests = []
    page.on("pageerror", lambda error: errors.append(str(error)))
    page.on("request", lambda request: external_requests.append(request.url) if not request.url.startswith(URL) else None)
    response = page.request.post(URL + "/api/config", form={"message": "FELIZ NATAL!", "wpm": "25"})
    assert response.status == 200
    page.goto(URL)
    expect(page.locator("#message")).to_have_value("FELIZ NATAL!")
    expect(page.locator("#save")).to_be_enabled()
    for label, width, height in [("desktop", 1440, 900), ("tablet", 1024, 768), ("mobile", 390, 844), ("narrow", 320, 740)]:
        page.set_viewport_size({"width": width, "height": height})
        assert page.evaluate("document.documentElement.scrollWidth <= window.innerWidth")
        page.screenshot(path=str(OUTPUT / f"ui-{label}.png"), full_page=True)
    # Client validation: rejected data is not posted and remains editable.
    page.locator("#message").fill("Feliz Natal, você!")
    page.locator("#save").click()
    expect(page.locator("#message-error")).to_be_visible()
    expect(page.locator("#message")).to_have_value("Feliz Natal, você!")
    page.locator("#message").fill("   ")
    page.locator("#save").click()
    expect(page.locator("#message-error")).to_have_text("Digite uma mensagem.")
    page.locator("#message").fill("  boas   festas!  ")
    page.locator("#wpm").fill("41")
    page.locator("#save").click()
    expect(page.locator("#wpm-error")).to_be_visible()
    page.locator("#wpm").fill("12.5")
    page.locator("#save").click()
    expect(page.locator("#wpm-error")).to_be_visible()
    page.locator("#wpm").fill("12")
    expect(page.locator("#speed-range")).to_have_value("12")
    page.locator("#speed-range").fill("40")
    expect(page.locator("#wpm")).to_have_value("40")
    page.locator("#save").click()
    expect(page.locator("#notice")).to_have_text("Configuração salva. A mensagem recomeçou.")
    expect(page.locator("#message")).to_have_value("BOAS FESTAS!")
    expect(page.locator("#current-message")).to_have_text("BOAS FESTAS!")
    page.reload()
    expect(page.locator("#message")).to_have_value("BOAS FESTAS!")
    expect(page.locator("#wpm")).to_have_value("40")
    # Long saved values must wrap on narrow displays.
    page.locator("#message").fill("E" * 120)
    page.locator("#save").click()
    expect(page.locator("#current-message")).to_have_text("E" * 120)
    assert page.evaluate("document.documentElement.scrollWidth <= window.innerWidth")
    # A slow save disables inputs and ignores a second submission.
    held = []
    def hold(route):
        held.append(route)
    page.route("**/api/config", hold)
    page.locator("#message").fill("SOS")
    page.locator("#save").click()
    expect(page.locator("#save")).to_be_disabled()
    expect(page.locator("#save")).to_have_text("Salvando…")
    page.evaluate("document.getElementById('config-form').requestSubmit()")
    assert len(held) == 1
    held.pop().continue_()
    expect(page.locator("#notice")).to_have_text("Configuração salva. A mensagem recomeçou.")
    page.unroute("**/api/config", hold)
    # Server/storage failure preserves draft and previous saved summary.
    def fail_storage(route):
        route.fulfill(status=500, content_type="application/json", body='{"message":"Não foi possível salvar. A configuração anterior foi mantida."}')
    page.route("**/api/config", fail_storage)
    page.locator("#message").fill("KEEP THIS DRAFT")
    page.locator("#save").click()
    expect(page.locator("#notice")).to_contain_text("configuração anterior")
    expect(page.locator("#message")).to_have_value("KEEP THIS DRAFT")
    expect(page.locator("#current-message")).to_have_text("SOS")
    expect(page.locator("#save")).to_be_enabled()
    page.unroute("**/api/config", fail_storage)
    # Losing the connection leaves the draft intact; retry succeeds later.
    page.route("**/api/config", lambda route: route.abort())
    page.locator("#save").click()
    expect(page.locator("#notice")).to_contain_text("Seus campos foram mantidos")
    expect(page.locator("#message")).to_have_value("KEEP THIS DRAFT")
    page.set_viewport_size({"width": 390, "height": 844})
    page.screenshot(path=str(OUTPUT / "ui-offline.png"), full_page=True)
    page.unroute("**/api/config")
    page.locator("#save").click()
    expect(page.locator("#notice")).to_have_text("Configuração salva. A mensagem recomeçou.")
    # Initial connection failure offers retry, without loading fake defaults.
    page.route("**/api/config", lambda route: route.abort())
    page.reload()
    expect(page.locator("#retry")).to_be_visible()
    expect(page.locator("#save")).to_be_disabled()
    page.unroute("**/api/config")
    page.locator("#retry").click()
    expect(page.locator("#save")).to_be_enabled()
    expect(page.locator("#message")).to_have_value("KEEP THIS DRAFT")
    assert not errors, errors
    assert not external_requests, external_requests
    # Native API checks through HTTP (not a fake JavaScript backend).
    assert page.request.post(URL + "/api/config", form={"message": "Olá", "wpm": "25"}).status == 400
    assert page.request.post(URL + "/api/config", data="x" * 1025, headers={"Content-Type": "application/x-www-form-urlencoded"}).status == 413
    page.request.post(URL + "/api/config", form={"message": "FELIZ NATAL!", "wpm": "25"})
    page.reload()
    expect(page.locator("#message")).to_have_value("FELIZ NATAL!")
    # Render editable hardware drawings for visual inspection.
    drawing = browser.new_page(viewport={"width": 1440, "height": 900})
    for name in ["schematic", "perfboard"]:
        drawing.set_content("<!doctype html><html><body style='margin:0'>" + (ROOT / f"docs/hardware/{name}.svg").read_text() + "</body></html>")
        drawing.screenshot(path=str(OUTPUT / f"hardware-{name}.png"), full_page=True)
    browser.close()
print("PASS responsive layout, save/reload, validation, duplicate submit, storage error, offline/retry, native HTTP API and self-contained assets")
