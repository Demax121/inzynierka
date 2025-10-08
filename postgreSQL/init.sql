--
-- PostgreSQL database dump
--

\restrict 6cR9p5UPXFE2u0fD7ZD4YX0GkfYjNFaeb9340Pbk9B07YGZfEIulwEuOtoPanPo

-- Dumped from database version 17.6 (Debian 17.6-2.pgdg13+1)
-- Dumped by pg_dump version 17.6

-- Started on 2025-10-05 18:39:59 UTC

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 217 (class 1259 OID 16385)
-- Name: blinds_config; Type: TABLE; Schema: public; Owner: postgresAdmin
--

CREATE TABLE public.blinds_config (
    "ID_config" integer NOT NULL,
    min_lux integer NOT NULL,
    max_lux integer NOT NULL,
    automate boolean
);


ALTER TABLE public.blinds_config OWNER TO "postgresAdmin";

--
-- TOC entry 218 (class 1259 OID 16388)
-- Name: devices; Type: TABLE; Schema: public; Owner: postgresAdmin
--

CREATE TABLE public.devices (
    device_id integer NOT NULL,
    device_name character varying(100) NOT NULL,
    device_mac macaddr NOT NULL,
    device_api_key character varying(16) NOT NULL,
    device_registered timestamp with time zone DEFAULT now() NOT NULL,
    device_last_seen timestamp with time zone DEFAULT now() NOT NULL,
    device_encryption_key character varying(32) NOT NULL
);


ALTER TABLE public.devices OWNER TO "postgresAdmin";

--
-- TOC entry 219 (class 1259 OID 16393)
-- Name: devices_device_id_seq; Type: SEQUENCE; Schema: public; Owner: postgresAdmin
--

CREATE SEQUENCE public.devices_device_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.devices_device_id_seq OWNER TO "postgresAdmin";

--
-- TOC entry 3470 (class 0 OID 0)
-- Dependencies: 219
-- Name: devices_device_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgresAdmin
--

ALTER SEQUENCE public.devices_device_id_seq OWNED BY public.devices.device_id;


--
-- TOC entry 220 (class 1259 OID 16395)
-- Name: door_status; Type: TABLE; Schema: public; Owner: postgresAdmin
--

CREATE TABLE public.door_status (
    status_id integer NOT NULL,
    status_date timestamp with time zone NOT NULL,
    status_name character varying NOT NULL
);


ALTER TABLE public.door_status OWNER TO "postgresAdmin";

--
-- TOC entry 221 (class 1259 OID 16400)
-- Name: door_status_status_id_seq; Type: SEQUENCE; Schema: public; Owner: postgresAdmin
--

ALTER TABLE public.door_status ALTER COLUMN status_id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME public.door_status_status_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- TOC entry 222 (class 1259 OID 16401)
-- Name: profiles; Type: TABLE; Schema: public; Owner: postgresAdmin
--

CREATE TABLE public.profiles (
    profile_id integer NOT NULL,
    profile_name character varying(100) NOT NULL,
    profile_json jsonb NOT NULL
);


ALTER TABLE public.profiles OWNER TO "postgresAdmin";

--
-- TOC entry 223 (class 1259 OID 16406)
-- Name: profiles_profile_id_seq; Type: SEQUENCE; Schema: public; Owner: postgresAdmin
--

CREATE SEQUENCE public.profiles_profile_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.profiles_profile_id_seq OWNER TO "postgresAdmin";

--
-- TOC entry 3471 (class 0 OID 0)
-- Dependencies: 223
-- Name: profiles_profile_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgresAdmin
--

ALTER SEQUENCE public.profiles_profile_id_seq OWNED BY public.profiles.profile_id;


--
-- TOC entry 224 (class 1259 OID 16408)
-- Name: room_statistics; Type: TABLE; Schema: public; Owner: postgresAdmin
--

CREATE TABLE public.room_statistics (
    stat_id integer NOT NULL,
    stat_date timestamp with time zone DEFAULT now() NOT NULL,
    stat_temperature real NOT NULL,
    stat_humidity real NOT NULL,
    stat_pressure real NOT NULL
);


ALTER TABLE public.room_statistics OWNER TO "postgresAdmin";

--
-- TOC entry 225 (class 1259 OID 16412)
-- Name: room_statistics_stat_id_seq; Type: SEQUENCE; Schema: public; Owner: postgresAdmin
--

CREATE SEQUENCE public.room_statistics_stat_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.room_statistics_stat_id_seq OWNER TO "postgresAdmin";

--
-- TOC entry 3472 (class 0 OID 0)
-- Dependencies: 225
-- Name: room_statistics_stat_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgresAdmin
--

ALTER SEQUENCE public.room_statistics_stat_id_seq OWNED BY public.room_statistics.stat_id;


--
-- TOC entry 3293 (class 2604 OID 16394)
-- Name: devices device_id; Type: DEFAULT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.devices ALTER COLUMN device_id SET DEFAULT nextval('public.devices_device_id_seq'::regclass);


--
-- TOC entry 3296 (class 2604 OID 16407)
-- Name: profiles profile_id; Type: DEFAULT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.profiles ALTER COLUMN profile_id SET DEFAULT nextval('public.profiles_profile_id_seq'::regclass);


--
-- TOC entry 3297 (class 2604 OID 16413)
-- Name: room_statistics stat_id; Type: DEFAULT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.room_statistics ALTER COLUMN stat_id SET DEFAULT nextval('public.room_statistics_stat_id_seq'::regclass);


--
-- TOC entry 3456 (class 0 OID 16385)
-- Dependencies: 217
-- Data for Name: blinds_config; Type: TABLE DATA; Schema: public; Owner: postgresAdmin
--

COPY public.blinds_config ("ID_config", min_lux, max_lux, automate) FROM stdin;
1	200	1200	f
\.


--
-- TOC entry 3457 (class 0 OID 16388)
-- Dependencies: 218
-- Data for Name: devices; Type: TABLE DATA; Schema: public; Owner: postgresAdmin
--

COPY public.devices (device_id, device_name, device_mac, device_api_key, device_registered, device_last_seen, device_encryption_key) FROM stdin;
2	sterownik_światło	4c:c3:82:cc:e1:08	kZ8UQmdrDar8	2025-09-04 09:52:30.765005+00	2025-10-05 15:35:08.682127+00	Vfyu3xT6e6yy79iE
5	sensor_drzwi	8c:d0:b2:a9:98:64	akr3ZPYDU5rM	2025-09-04 09:52:30.765005+00	2025-10-05 14:13:37.915043+00	2zBxk8uBiQd7p32T
3	sensor_lux	4c:c3:82:d0:cb:0c	9ekJU68REYTR	2025-09-04 09:52:30.765005+00	2025-10-05 15:20:18.594146+00	J247J3LBDegpAUaU
1	sterownik_klima	84:1f:e8:1b:35:dc	NfzziMcV9Zyj	2025-09-04 09:52:30.765005+00	2025-10-05 15:39:18.753874+00	KQAzhJmqigFdUyD6
4	sensor_temperatura	34:cd:b0:cd:f3:10	b3odiEjCNSf7	2025-09-04 09:52:30.765005+00	2025-10-05 18:39:51.073216+00	xuCmb33pFJgJAwR5
\.


--
-- TOC entry 3459 (class 0 OID 16395)
-- Dependencies: 220
-- Data for Name: door_status; Type: TABLE DATA; Schema: public; Owner: postgresAdmin
--

COPY public.door_status (status_id, status_date, status_name) FROM stdin;
1	2025-10-03 19:10:51.054496+00	Door closed
2	2025-10-03 19:10:53.286906+00	Door open
3	2025-10-03 19:53:38.297896+00	Door closed
4	2025-10-03 19:53:39.414919+00	Door open
5	2025-10-03 20:54:58.582733+00	Door closed
6	2025-10-03 20:54:59.06597+00	Door open
7	2025-10-03 20:54:59.865849+00	Door closed
8	2025-10-03 20:54:59.995504+00	Door open
9	2025-10-04 17:53:58.661167+00	Door closed
10	2025-10-04 17:53:59.405044+00	Door open
11	2025-10-04 20:41:00.513036+00	Door closed
12	2025-10-04 20:41:00.738447+00	Door open
13	2025-10-04 20:46:11.712637+00	Door open
14	2025-10-04 20:46:11.714641+00	Door open
15	2025-10-04 21:11:49.553466+00	Door closed
16	2025-10-04 21:11:49.867205+00	Door closed
17	2025-10-04 21:11:51.384936+00	Door open
18	2025-10-04 21:11:51.791906+00	Door open
19	2025-10-04 21:30:29.318127+00	Door closed
20	2025-10-04 21:30:29.798125+00	Door open
21	2025-10-04 21:30:30.259679+00	Door closed
22	2025-10-04 21:30:30.598725+00	Door open
23	2025-10-04 21:30:30.931988+00	Door closed
24	2025-10-04 21:30:31.080604+00	Door open
25	2025-10-04 21:30:31.275835+00	Door closed
26	2025-10-04 21:30:31.395245+00	Door open
27	2025-10-04 21:30:31.851836+00	Door closed
28	2025-10-04 21:30:31.955593+00	Door open
29	2025-10-04 21:30:32.055159+00	Door closed
30	2025-10-04 21:30:32.17139+00	Door open
31	2025-10-05 10:20:34.021354+00	Door closed
32	2025-10-05 10:20:35.495034+00	Door open
33	2025-10-05 10:24:36.137082+00	Door closed
34	2025-10-05 10:24:37.124356+00	Door open
35	2025-10-05 10:36:58.835091+00	Door closed
36	2025-10-05 10:36:59.988255+00	Door open
37	2025-10-05 10:37:51.487661+00	Door closed
38	2025-10-05 10:37:52.369125+00	Door open
39	2025-10-05 10:41:22.992384+00	Door closed
40	2025-10-05 10:41:23.864142+00	Door open
\.


--
-- TOC entry 3461 (class 0 OID 16401)
-- Dependencies: 222
-- Data for Name: profiles; Type: TABLE DATA; Schema: public; Owner: postgresAdmin
--

COPY public.profiles (profile_id, profile_name, profile_json) FROM stdin;
8	Work	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 24}}, "WLED": {"on": false, "preset_name": "Wyłączony"}, "blinds": {"state": "open"}, "lights": {"channel": "main_lights", "payload": {"state": true}}}
9	Japan	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 27}}, "WLED": {"on": true, "ps": 2, "lor": 2, "preset_name": "Cherry blossom"}, "blinds": {"state": "close"}, "lights": {"channel": "main_lights", "payload": {"state": false}}}
11	Fall	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 27}}, "WLED": {"on": true, "ps": 3, "lor": 2, "preset_name": "The Fall"}, "blinds": {"state": "close"}, "lights": {"channel": "main_lights", "payload": {"state": false}}}
19	Test	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 26.5}}, "WLED": {"on": true, "lor": 0, "preset_name": "Ambilight"}, "blinds": {"state": "close"}, "lights": {"channel": "main_lights", "payload": {"state": true}}}
\.


--
-- TOC entry 3463 (class 0 OID 16408)
-- Dependencies: 224
-- Data for Name: room_statistics; Type: TABLE DATA; Schema: public; Owner: postgresAdmin
--

COPY public.room_statistics (stat_id, stat_date, stat_temperature, stat_humidity, stat_pressure) FROM stdin;
1	2025-10-05 16:29:21.856931+00	22.91	49.87109	1006.038
2	2025-10-05 16:30:18.957011+00	22.98	49.96777	1006.041
3	2025-10-05 16:38:58.376729+00	23.04	49.86426	1006.073
4	2025-10-05 16:39:37.488835+00	23.05	49.73242	1006.099
5	2025-10-05 17:39:38.263539+00	23.01	49.69434	1006.36
6	2025-10-05 18:39:38.411309+00	22.9	50.7627	1006.807
\.


--
-- TOC entry 3473 (class 0 OID 0)
-- Dependencies: 219
-- Name: devices_device_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgresAdmin
--

SELECT pg_catalog.setval('public.devices_device_id_seq', 5, true);


--
-- TOC entry 3474 (class 0 OID 0)
-- Dependencies: 221
-- Name: door_status_status_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgresAdmin
--

SELECT pg_catalog.setval('public.door_status_status_id_seq', 40, true);


--
-- TOC entry 3475 (class 0 OID 0)
-- Dependencies: 223
-- Name: profiles_profile_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgresAdmin
--

SELECT pg_catalog.setval('public.profiles_profile_id_seq', 19, true);


--
-- TOC entry 3476 (class 0 OID 0)
-- Dependencies: 225
-- Name: room_statistics_stat_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgresAdmin
--

SELECT pg_catalog.setval('public.room_statistics_stat_id_seq', 6, true);


--
-- TOC entry 3300 (class 2606 OID 16415)
-- Name: blinds_config blinds_config_pkey; Type: CONSTRAINT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.blinds_config
    ADD CONSTRAINT blinds_config_pkey PRIMARY KEY ("ID_config");


--
-- TOC entry 3302 (class 2606 OID 16417)
-- Name: devices devices_device_apikey_key; Type: CONSTRAINT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_device_apikey_key UNIQUE (device_api_key);


--
-- TOC entry 3304 (class 2606 OID 16419)
-- Name: devices devices_device_encryption_key_key; Type: CONSTRAINT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_device_encryption_key_key UNIQUE (device_encryption_key);


--
-- TOC entry 3306 (class 2606 OID 16421)
-- Name: devices devices_device_mac_key; Type: CONSTRAINT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_device_mac_key UNIQUE (device_mac);


--
-- TOC entry 3308 (class 2606 OID 16423)
-- Name: devices devices_pkey; Type: CONSTRAINT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_pkey PRIMARY KEY (device_id);


--
-- TOC entry 3310 (class 2606 OID 16425)
-- Name: profiles profiles_pkey; Type: CONSTRAINT; Schema: public; Owner: postgresAdmin
--

ALTER TABLE ONLY public.profiles
    ADD CONSTRAINT profiles_pkey PRIMARY KEY (profile_id);


-- Completed on 2025-10-05 18:39:59 UTC

--
-- PostgreSQL database dump complete
--

\unrestrict 6cR9p5UPXFE2u0fD7ZD4YX0GkfYjNFaeb9340Pbk9B07YGZfEIulwEuOtoPanPo

