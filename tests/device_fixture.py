"""Drive the real connection handshake through production receive handling."""
def parameter_snapshot():
    values = (1.2, 1.2, 0, 0, 30, 30, 4, 160000, 100000, -160000, 160000,
              -100000, 100000, 1, 300000, 20000, 10000, 15000, 0, 300000, 20000, 6400)
    return "PL " + ";".join(f"{35+i},{value}" for i, value in enumerate(values))


def ready(window, source="TCP", extended=False):
    client=window.command_client if source=="TCP" else window.serial_client
    client.is_connected=lambda:True
    (window._on_connected if source=="TCP" else window._on_uart_connected)()
    responses=["VERSION 1.0","GSTA 0,1,1,0,E0000,","SPEED SPEED 100",
               "PCONFIG 1,35" if extended else "PCONFIG UNSUPPORTED"]
    if extended:
        responses.extend(("GSTATE 0,1,0,0,0,1,E0000,0,0", parameter_snapshot(),
                          "GSENSOR 1,1,0,0,1,1,0,1,0"))
    for response in responses:
        window._log_received((response+"\r\n").encode(),source)
    session=window.sessions[source]
    if session.disconnected_during_motion:
        session.request("STOP")
        state = "GSTATE 0,1,0,0,0,1,E0000,0,0" if extended else "GSTA 0,1,1,0,E0000"
        window._log_received(f"STOP stop\r\n{state}\r\n".encode(), source)
    return session
