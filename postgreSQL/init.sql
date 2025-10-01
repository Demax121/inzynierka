
\restrict 6Bv8kFpMrLK66OgTJ2izjP0e1z3ZeIocMoT1ES7cyuQAEKIQsVvfh5m5rRymcVc

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

-- Table: blinds_config
CREATE TABLE public.blinds_config (
    "ID_config" integer NOT NULL,
    min_lux integer NOT NULL,
    max_lux integer NOT NULL,
    automate boolean
);
ALTER TABLE public.blinds_config OWNER TO "postgresAdmin";

-- Table: devices
CREATE TABLE public.devices (
    device_id integer NOT NULL,
    device_name character varying(100) NOT NULL,
    device_mac macaddr NOT NULL,
    device_api_key character varying(32) NOT NULL,
    device_registered timestamp with time zone DEFAULT now() NOT NULL,
    device_last_seen timestamp with time zone DEFAULT now() NOT NULL,
    device_encryption_key character varying(64) NOT NULL
);
ALTER TABLE public.devices OWNER TO "postgresAdmin";

CREATE SEQUENCE public.devices_device_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
ALTER SEQUENCE public.devices_device_id_seq OWNER TO "postgresAdmin";
ALTER SEQUENCE public.devices_device_id_seq OWNED BY public.devices.device_id;
ALTER TABLE ONLY public.devices ALTER COLUMN device_id SET DEFAULT nextval('public.devices_device_id_seq'::regclass);

-- Table: door_status
CREATE TABLE public.door_status (
    status_id integer NOT NULL,
    status_date timestamp with time zone NOT NULL,
    status_name character varying NOT NULL
);
ALTER TABLE public.door_status OWNER TO "postgresAdmin";
ALTER TABLE public.door_status ALTER COLUMN status_id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME public.door_status_status_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);

-- Table: profiles
CREATE TABLE public.profiles (
    profile_id integer NOT NULL,
    profile_name character varying(100) NOT NULL,
    profile_json jsonb NOT NULL
);
ALTER TABLE public.profiles OWNER TO "postgresAdmin";

CREATE SEQUENCE public.profiles_profile_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
ALTER SEQUENCE public.profiles_profile_id_seq OWNER TO "postgresAdmin";
ALTER SEQUENCE public.profiles_profile_id_seq OWNED BY public.profiles.profile_id;
ALTER TABLE ONLY public.profiles ALTER COLUMN profile_id SET DEFAULT nextval('public.profiles_profile_id_seq'::regclass);

-- Table: room_statistics
CREATE TABLE public.room_statistics (
    stat_id integer NOT NULL,
    stat_date timestamp with time zone DEFAULT now() NOT NULL,
    stat_temperature real NOT NULL,
    stat_humidity real NOT NULL,
    stat_pressure real NOT NULL
);
ALTER TABLE public.room_statistics OWNER TO "postgresAdmin";
CREATE SEQUENCE public.room_statistics_stat_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;
ALTER SEQUENCE public.room_statistics_stat_id_seq OWNER TO "postgresAdmin";
ALTER SEQUENCE public.room_statistics_stat_id_seq OWNED BY public.room_statistics.stat_id;
ALTER TABLE ONLY public.room_statistics ALTER COLUMN stat_id SET DEFAULT nextval('public.room_statistics_stat_id_seq'::regclass);

-- Data: blinds_config
COPY public.blinds_config ("ID_config", min_lux, max_lux, automate) FROM stdin;
1	500	2000	f
\.

-- Data: devices
COPY public.devices (device_id, device_name, device_mac, device_api_key, device_registered, device_last_seen, device_encryption_key) FROM stdin;
2	sterownik_światło	4c:c3:82:cc:e1:08	bkbhbwd2grxqnvihqnspq3hv7d28chae	2025-09-04 09:52:30.765005+00	2025-09-04 09:52:30.765005+00	2fe32bb098934a0d7a9e6c9536a91a45feb8c873611a86cfadf2e069f060f404
4	sensor_temperatura	34:cd:b0:cd:f3:10	v36pqhzdxkjy6f729euank9bzhxupk3i	2025-09-04 09:52:30.765005+00	2025-09-04 09:52:30.765005+00	c58bcbf5b7ee423c717efe9fe6d6c19f4ff12bbcb27175403aeff82b91167718
5	sensor_drzwi	8c:d0:b2:a9:98:64	zmabu3h4e8rdzmmwmjg23jvodxbt6p76	2025-09-04 09:52:30.765005+00	2025-09-04 09:52:30.765005+00	e91430e4368613e3cc62cbf8ef95df649fcca48ea20e17e4ddd9ec97a442cfd9
1	sterownik_klima	84:1f:e8:1b:35:dc	s72ak72ohwmyqp7wkfc7zdf57ki8t3w2	2025-09-04 09:52:30.765005+00	2025-09-04 09:52:30.765005+00	dfb9a7006b95ba94a853dc1cac2e7b0794044dfdf47539460b535750f7bfee7f
3	sensor_lux	4c:c3:82:d0:cb:0c	ksnf4o8a9uq9a46r9hcr2siisk7o244x	2025-09-04 09:52:30.765005+00	2025-09-04 09:52:30.765005+00	648f11ffc8a1a5489bc144b15b630c48ddb3d2f9e8234697fddf14ea36fe577d
\.

-- Data: profiles
COPY public.profiles (profile_id, profile_name, profile_json) FROM stdin;
8	Work	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 24}}, "WLED": {"on": false, "preset_name": "Wyłączony"}, "blinds": {"state": "open"}, "lights": {"channel": "main_lights", "payload": {"state": true}}}
9	Japan	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 27}}, "WLED": {"on": true, "ps": 2, "lor": 2, "preset_name": "Cherry blossom"}, "blinds": {"state": "close"}, "lights": {"channel": "main_lights", "payload": {"state": false}}}
11	Fall	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 27}}, "WLED": {"on": true, "ps": 3, "lor": 2, "preset_name": "The Fall"}, "blinds": {"state": "close"}, "lights": {"channel": "main_lights", "payload": {"state": false}}}
12	Test	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 26}}, "WLED": {"on": false, "preset_name": "Wyłączony"}, "blinds": {"maxLux": 2000, "minLux": 500, "automate": true}, "lights": {"channel": "main_lights", "payload": {"state": true}}}
13	Zielono mi	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 26}}, "WLED": {"on": true, "ps": 5, "lor": 2, "preset_name": "Fairy Forest"}, "blinds": {"state": "open"}, "lights": {"channel": "main_lights", "payload": {"state": false}}}
14	Pietruszka	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 26}}, "WLED": {"on": true, "lor": 0, "preset_name": "Ambilight"}, "blinds": {"state": "close"}, "lights": {"channel": "main_lights", "payload": {"state": false}}}
15	test lapek	{"AC": {"channel": "air_conditioning", "payload": {"requestedTemp": 22}}, "WLED": {"on": true, "lor": 0, "preset_name": "Ambilight"}, "blinds": {"state": "close"}, "lights": {"channel": "main_lights", "payload": {"state": true}}}
\.

-- Sequences set
SELECT pg_catalog.setval('public.devices_device_id_seq', 5, true);
SELECT pg_catalog.setval('public.profiles_profile_id_seq', 15, true);

-- Constraints
ALTER TABLE ONLY public.blinds_config
    ADD CONSTRAINT blinds_config_pkey PRIMARY KEY ("ID_config");

ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_device_apikey_key UNIQUE (device_api_key);
ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_device_encryption_key_key UNIQUE (device_encryption_key);
ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_device_mac_key UNIQUE (device_mac);
ALTER TABLE ONLY public.devices
    ADD CONSTRAINT devices_pkey PRIMARY KEY (device_id);

ALTER TABLE ONLY public.profiles
    ADD CONSTRAINT profiles_pkey PRIMARY KEY (profile_id);

\unrestrict 6Bv8kFpMrLK66OgTJ2izjP0e1z3ZeIocMoT1ES7cyuQAEKIQsVvfh5m5rRymcVc
